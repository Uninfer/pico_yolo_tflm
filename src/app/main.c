#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/sync.h"

#include "board_pins.h"
#include "camera_capture.h"
#include "config.h"
#include "lcd.h"
#include "lcd_config.h"
#include "main_functions.h"
#include "ov5640_camera.h"

#define FRAME_WIDTH BOARD_LCD_WIDTH
#define FRAME_HEIGHT BOARD_LCD_HEIGHT

static const uint16_t colors[] = {
    0x07E0, // green
    0xF800, // red
    0x001F, // blue
    0xFFE0, // yellow
    0xFFFF, // white
};

// This board has room for one camera frame plus the TFLM arena. Semaphores
// transfer ownership of the frame between capture/display and preprocessing.
__attribute__((section(".sram0"), aligned(4)))
static uint16_t camera_frame[FRAME_WIDTH * FRAME_HEIGHT];
static uint16_t *image_buffer = camera_frame;

semaphore_t g_inference_request;
semaphore_t g_frame_buffer_consumed;
static mutex_t g_inference_lock;

static BBox share_boxes[MAX_BOXES];
static int share_box_count;
static bool inference_enabled;
static bool inference_busy;

static void draw_box(void)
{
    BBox boxes[MAX_BOXES];
    int box_count;

    // Copy the most recent result quickly, then draw without holding the lock
    // while SPI transfers pixels to the LCD.
    mutex_enter_blocking(&g_inference_lock);
    box_count = share_box_count;
    memcpy(boxes, share_boxes, (size_t)box_count * sizeof(boxes[0]));
    mutex_exit(&g_inference_lock);

    for (int i = 0; i < box_count; ++i) {
        const BBox *box = &boxes[i];
        if (box->class_id < 0 || box->class_id >= (int)(sizeof(colors) / sizeof(colors[0]))) {
            continue;
        }

        int x1 = (int)(box->x1 * FRAME_WIDTH);
        int y1 = (int)(box->y1 * FRAME_HEIGHT);
        int x2 = (int)(box->x2 * FRAME_WIDTH);
        int y2 = (int)(box->y2 * FRAME_HEIGHT);
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 >= FRAME_WIDTH) x2 = FRAME_WIDTH - 1;
        if (y2 >= FRAME_HEIGHT) y2 = FRAME_HEIGHT - 1;
        if (x1 > x2 || y1 > y2) {
            continue;
        }

        draw_boxes_to_lcd(x1, y1, x2, y2, colors[box->class_id]);
    }
}

static void core1_entry(void)
{
    BBox boxes[MAX_BOXES];
    int box_count;

    for (;;) {
        sem_acquire_blocking(&g_inference_request);
        box_count = 0;
        pico_yolo_infer(image_buffer, boxes, &box_count);

        mutex_enter_blocking(&g_inference_lock);
        if (box_count < 0) {
            box_count = 0;
        } else if (box_count > MAX_BOXES) {
            box_count = MAX_BOXES;
        }
        memcpy(share_boxes, boxes, (size_t)box_count * sizeof(boxes[0]));
        share_box_count = box_count;
        inference_busy = false;
        mutex_exit(&g_inference_lock);
    }
}

static void configure_clocks(void)
{
#ifndef PICO_YOLO_SYS_CLOCK_KHZ
#define PICO_YOLO_SYS_CLOCK_KHZ 300000
#endif
    const uint32_t requested_clock_khz = PICO_YOLO_SYS_CLOCK_KHZ;

    if (requested_clock_khz > 150000) {
        // Above the RP2350's documented 150 MHz baseline, raise VREG only to
        // the SDK's maximum supported setting before attempting the clock.
        vreg_set_voltage(VREG_VOLTAGE_MAX);
        sleep_ms(10);
    }

    if (!set_sys_clock_khz(requested_clock_khz, false)) {
        printf("Could not set %lu kHz; falling back to 150 MHz.\n",
               (unsigned long)requested_clock_khz);
        set_sys_clock_khz(150000, true);
    }

    const uint32_t sys_hz = clock_get_hz(clk_sys);
    if (!clock_configure(clk_peri, 0,
                         CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                         sys_hz, sys_hz)) {
        printf("Warning: could not match clk_peri to clk_sys.\n");
    }
    printf("System clock: %lu Hz, peripheral clock: %lu Hz\n",
           (unsigned long)clock_get_hz(clk_sys),
           (unsigned long)clock_get_hz(clk_peri));
}

int main(void)
{
    stdio_init_all();
    configure_clocks();

    // No frame or inference request exists until the first camera DMA finishes.
    sem_init(&g_inference_request, 0, 1);
    sem_init(&g_frame_buffer_consumed, 0, 1);
    mutex_init(&g_inference_lock);

    inference_enabled = pico_yolo_init();
    lcd_init();
    ov5640_camera_init(image_buffer);
    // The PIO program waits for both count words before it can capture frame 1.
    ov5640_camera_request_frame();
    if (inference_enabled) {
        multicore_launch_core1(core1_entry);
    } else {
        printf("TFLM initialization failed; camera preview will continue without detections.\n");
    }

    for (;;) {
        while (!ov5640_camera_frame_ready()) {
            tight_loop_contents();
        }
        ov5640_camera_ack_frame();

        // Schedule inference only when Core 1 is idle. If it is still running
        // the previous model invocation, skip this frame's inference so the
        // live preview does not wait for TFLM.
        bool inference_requested = false;
        if (inference_enabled) {
            mutex_enter_blocking(&g_inference_lock);
            if (!inference_busy) {
                inference_busy = true;
                inference_requested = true;
            }
            mutex_exit(&g_inference_lock);

            if (inference_requested) {
                sem_release(&g_inference_request);
            }
        }

#ifdef CONFIG_LCD_USE_DMA
        lcd_show_image_dma(image_buffer);
#else
        lcd_show_image(image_buffer);
#endif
        draw_box();

        // Preprocessing reads the camera frame, then copies it into TFLM's
        // tensor. Wait only for that copy before reusing the single frame
        // buffer; the model invocation can overlap the next camera capture.
        if (inference_requested) {
            sem_acquire_blocking(&g_frame_buffer_consumed);
        }
        ov5640_camera_request_frame();
    }
}
