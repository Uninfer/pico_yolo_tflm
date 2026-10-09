#include "pico_yolo_inference.h"

#include "camera_pipeline.h"
#include "lcd.h"

#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "pico/sync.h"

#include <stdio.h>
#include <string.h>

#ifndef PICO_YOLO_FRAME_BUFFER_COUNT
#define PICO_YOLO_FRAME_BUFFER_COUNT 2U
#endif
#define FRAME_BUFFER_COUNT PICO_YOLO_FRAME_BUFFER_COUNT
/* 超出有效帧索引的值表示当前没有使用帧。 */
#define NO_FRAME FRAME_BUFFER_COUNT
#define FRAME_PIXELS (PICO_YOLO_FRAME_WIDTH * PICO_YOLO_FRAME_HEIGHT)

static const uint16_t g_colors[] = {
    0x07E0U, /* fire: green */
    0xFFE0U  /* smoke: yellow */
};

static mutex_t g_frame_lock;
static mutex_t g_box_lock;
/* Keep both ping-pong frames on identical cache/SRAM-bank alignment.  The
   old extra 12-byte stride made alternating frames take different paths. */
static uint16_t g_frame_storage[FRAME_BUFFER_COUNT][FRAME_PIXELS] __attribute__((aligned(64)));
/* The reused camera driver references this board-level symbol directly. */
uint16_t *image_buffer = g_frame_storage[0];
static pico_yolo_box_t g_shared_boxes[PICO_YOLO_MAX_BOXES];
static uint32_t g_shared_box_count;
static volatile uint32_t g_latest_frame_index;
static volatile uint32_t g_latest_frame_sequence;
static bool g_frame_available;
static volatile uint32_t g_inference_reader_index = NO_FRAME;
static uint32_t g_display_reader_index = NO_FRAME;
static uint64_t g_frame_capture_started[FRAME_BUFFER_COUNT];
static uint64_t g_result_capture_started;
static uint32_t g_result_sequence;
static uint64_t g_displayed_capture_started;
static uint32_t g_displayed_sequence;

void pico_yolo_wait_for_display(void)
{
    for (;;) {
        mutex_enter_blocking(&g_frame_lock);
        const bool ready = g_inference_reader_index == NO_FRAME ||
                           g_display_reader_index != g_inference_reader_index;
        mutex_exit(&g_frame_lock);
        if (ready) {
            return;
        }
        tight_loop_contents();
    }
}

void pico_yolo_release_frame(void)
{
    mutex_enter_blocking(&g_frame_lock);
    g_inference_reader_index = NO_FRAME;
    mutex_exit(&g_frame_lock);
}

static void frame_diagnostics(const uint16_t *frame, uint16_t *minimum, uint16_t *maximum,
                              uint32_t *checksum)
{
    uint32_t index;
    uint32_t hash = 2166136261U;
    uint16_t low = 0xffffU;
    uint16_t high = 0U;
    for (index = 0U; index < FRAME_PIXELS; index += 17U) {
        const uint16_t pixel = frame[index];
        if (pixel < low) {
            low = pixel;
        }
        if (pixel > high) {
            high = pixel;
        }
        hash = (hash ^ pixel) * 16777619U;
    }
    *minimum = low;
    *maximum = high;
    *checksum = hash;
}

static void draw_detections(void)
{
    pico_yolo_box_t boxes[PICO_YOLO_MAX_BOXES];
    uint32_t count;
    uint32_t index;
    mutex_enter_blocking(&g_box_lock);
    count = g_shared_box_count;
    g_displayed_sequence = g_result_sequence;
    g_displayed_capture_started = g_result_capture_started;
    if (count != 0U) {
        memcpy(boxes, g_shared_boxes, count * sizeof(boxes[0]));
    }
    mutex_exit(&g_box_lock);
    for (index = 0U; index < count; ++index) {
        const pico_yolo_box_t *box = &boxes[index];
        draw_boxes_to_lcd((int)box->x1, (int)box->y1, (int)box->x2, (int)box->y2,
                          g_colors[box->class_id < 2U ? box->class_id : 0U]);
    }
}

/* Core 1：取最新帧 -> 预处理 -> 推理 -> 发布检测框。
 * 帧锁保护谁正在读哪个缓冲区；框锁保护检测结果。 */
static void core1_entry(void)
{
    pico_yolo_box_t boxes[PICO_YOLO_MAX_BOXES];
    uint32_t processed_sequence = 0U;
    uint32_t inference_count = 0U;
    uint64_t preprocess_total = 0U;
    uint64_t inference_total = 0U;
    uint64_t postprocess_total = 0U;
    uint64_t report_started = time_us_64();
    while (true) {
        uint32_t frame_index = NO_FRAME;
        uint32_t sequence = processed_sequence;
        uint32_t box_count = 0U;
        uint32_t preprocess_us = 0U;
        uint32_t inference_us = 0U;
        uint32_t postprocess_us = 0U;
        uint64_t capture_started = 0U;

        while (frame_index == NO_FRAME) {
            mutex_enter_blocking(&g_frame_lock);
            if (g_frame_available && g_latest_frame_sequence != processed_sequence) {
                frame_index = g_latest_frame_index;
                sequence = g_latest_frame_sequence;
                capture_started = g_frame_capture_started[frame_index];
                g_inference_reader_index = frame_index;
            }
            mutex_exit(&g_frame_lock);
            if (frame_index == NO_FRAME) {
                tight_loop_contents();
            }
        }
        if (pico_yolo_preprocess_frame(g_frame_storage[frame_index], &preprocess_us) != 0) {
            pico_yolo_release_frame();
            processed_sequence = sequence;
            printf("Preprocessing failed\n");
            continue;
        }
        processed_sequence = sequence;

        if (pico_yolo_run_preprocessed(boxes, &box_count, &inference_us, &postprocess_us) == 0) {
            mutex_enter_blocking(&g_box_lock);
            g_shared_box_count = box_count;
            g_result_capture_started = capture_started;
            ++g_result_sequence;
            if (box_count != 0U) {
                memcpy(g_shared_boxes, boxes, box_count * sizeof(boxes[0]));
            }
            mutex_exit(&g_box_lock);
        } else {
            printf("Model invocation failed\n");
            pico_yolo_release_frame();
            continue;
        }
        ++inference_count;
        preprocess_total += preprocess_us;
        inference_total += inference_us;
        postprocess_total += postprocess_us;
        if (time_us_64() - report_started >= 1000000U) {
            const uint64_t elapsed = time_us_64() - report_started;
            printf("INFER fps=%lu.%lu pre=%lu us model=%lu us post=%lu us boxes=%lu window=%llu us "
                   "count=%lu\n",
                   (unsigned long)(inference_count * 10000000ULL / elapsed / 10U),
                   (unsigned long)(inference_count * 10000000ULL / elapsed % 10U),
                   (unsigned long)(preprocess_total / inference_count),
                   (unsigned long)(inference_total / inference_count),
                   (unsigned long)(postprocess_total / inference_count), (unsigned long)box_count,
                   (unsigned long long)elapsed, (unsigned long)inference_count);
            inference_count = 0U;
            preprocess_total = 0U;
            inference_total = 0U;
            postprocess_total = 0U;
            report_started = time_us_64();
        }
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
    if (!clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, sys_hz,
                         sys_hz)) {
        printf("Warning: could not match clk_peri to clk_sys.\n");
    }
    printf("System clock: %lu Hz, peripheral clock: %lu Hz\n", (unsigned long)clock_get_hz(clk_sys),
           (unsigned long)clock_get_hz(clk_peri));
}

int main(void)
{
    uint32_t usb_wait_ms = 0U;
    stdio_init_all();
    while (!stdio_usb_connected() && usb_wait_ms < 5000U) {
        sleep_ms(10);
        usb_wait_ms += 10U;
    }
    configure_clocks();
    mutex_init(&g_frame_lock);
    mutex_init(&g_box_lock);

    printf("\nPico YOLO boot\n");
    printf("RP2350 clock: %lu Hz\n", clock_get_hz(clk_sys));
    if (pico_yolo_init() != 0) {
        printf("Fatal: model initialization failed\n");
        while (true) {
            tight_loop_contents();
        }
    }

    lcd_init();
    printf("LCD path: blocking (reference-compatible)\n");
    camera_pipeline_init(image_buffer);
    uint64_t capture_started = time_us_64();
    printf("Camera initialized: %ux%u portrait RGB565\n", (unsigned)PICO_YOLO_FRAME_WIDTH,
           (unsigned)PICO_YOLO_FRAME_HEIGHT);
    multicore_launch_core1(core1_entry);
    printf("Core 1 launched\n");

    uint32_t capture_index = 0U;
    uint32_t display_index = FRAME_BUFFER_COUNT - 1U;
    uint32_t display_count = 0U;
    uint64_t lcd_total_us = 0U;
    uint64_t display_total_us = 0U;
    uint64_t capture_total_us = 0U;
    uint64_t chain_total_us = 0U;
    uint32_t fresh_count = 0U;
    uint32_t last_displayed_sequence = 0U;
    uint64_t report_started = time_us_64();
    while (true) {
        while (!camera_pipeline_frame_ready()) {
            tight_loop_contents();
        }
        capture_total_us += time_us_64() - capture_started;

        /* 等推理释放即将复用的缓冲区，再交换采集帧与显示帧。 */
        while (true) {
            uint32_t reader;
            mutex_enter_blocking(&g_frame_lock);
            reader = g_inference_reader_index;
            if (reader != display_index) {
                const uint32_t old_display = display_index;
                display_index = capture_index;
                capture_index = old_display;
                g_display_reader_index = display_index;
                g_latest_frame_index = display_index;
                g_frame_capture_started[display_index] = capture_started;
                ++g_latest_frame_sequence;
                g_frame_available = true;
                mutex_exit(&g_frame_lock);
                break;
            }
            mutex_exit(&g_frame_lock);
            tight_loop_contents();
        }

#if PICO_YOLO_FRAME_BUFFER_COUNT == 2
        capture_started = time_us_64();
        /* 双帧：采集与显示使用不同缓冲区，可以同时进行。 */
        camera_pipeline_capture_to(g_frame_storage[capture_index]);
#endif
        const uint64_t lcd_started = time_us_64();
        /* 阻塞式显示返回后，屏幕才不再读取这块内存。 */
        lcd_show_image(g_frame_storage[display_index]);
        lcd_total_us += time_us_64() - lcd_started;
        draw_detections();
        const uint64_t displayed_us = time_us_64();
        display_total_us += displayed_us - lcd_started;
        if (g_displayed_sequence != last_displayed_sequence) {
            ++fresh_count;
            chain_total_us += displayed_us - g_displayed_capture_started;
            last_displayed_sequence = g_displayed_sequence;
        }
        ++display_count;

        if (time_us_64() - report_started >= 1000000U) {
            const uint64_t elapsed = time_us_64() - report_started;
            const uint64_t fps10 = display_count * 10000000ULL / elapsed;
            const uint64_t preview_fps100 = display_count * 100000000ULL / elapsed;
            const uint64_t fresh_fps100 = fresh_count * 100000000ULL / elapsed;
            printf("PIPE window %llu us | preview %lu frames %llu.%02llu fps | fresh %lu results "
                   "%llu.%02llu fps | capture %llu us | lcd %llu us | chain %llu us\n",
                   (unsigned long long)elapsed, (unsigned long)display_count,
                   (unsigned long long)(preview_fps100 / 100U),
                   (unsigned long long)(preview_fps100 % 100U), (unsigned long)fresh_count,
                   (unsigned long long)(fresh_fps100 / 100U),
                   (unsigned long long)(fresh_fps100 % 100U),
                   (unsigned long long)(capture_total_us / display_count),
                   (unsigned long long)(display_total_us / display_count),
                   (unsigned long long)(fresh_count ? chain_total_us / fresh_count : 0U));
            uint16_t frame_minimum;
            uint16_t frame_maximum;
            uint16_t gain_x16;
            uint32_t exposure_x16;
            uint32_t frame_checksum;
            frame_diagnostics(g_frame_storage[display_index], &frame_minimum, &frame_maximum,
                              &frame_checksum);
            camera_pipeline_read_exposure(&gain_x16, &exposure_x16);
            printf("VIDEO fps=%lu.%lu lcd=%lu us frame=[%04x..%04x] crc=%08lx gain=%lu.%lux "
                   "exp=%lu.%lu lines\n",
                   (unsigned long)(fps10 / 10U), (unsigned long)(fps10 % 10U),
                   (unsigned long)(lcd_total_us / display_count), frame_minimum, frame_maximum,
                   (unsigned long)frame_checksum, (unsigned long)(gain_x16 / 16U),
                   (unsigned long)(gain_x16 * 10U / 16U % 10U), (unsigned long)(exposure_x16 / 16U),
                   (unsigned long)(exposure_x16 * 10U / 16U % 10U));
            display_count = 0U;
            lcd_total_us = 0U;
            display_total_us = 0U;
            capture_total_us = 0U;
            chain_total_us = 0U;
            fresh_count = 0U;
            report_started = time_us_64();
        }
        /* 诊断也会读取图像，所以最后才允许推理覆盖这帧。 */
        mutex_enter_blocking(&g_frame_lock);
        g_display_reader_index = NO_FRAME;
        mutex_exit(&g_frame_lock);
#if PICO_YOLO_FRAME_BUFFER_COUNT == 1
        /* 单帧：屏幕和预处理都读完后，才能启动下一次采集。 */
        while (true) {
            mutex_enter_blocking(&g_frame_lock);
            const bool released = g_inference_reader_index == NO_FRAME;
            if (released) {
                /* DMA 期间不允许推理重新取得这块正在写入的帧。 */
                g_frame_available = false;
            }
            mutex_exit(&g_frame_lock);
            if (released) {
                break;
            }
            tight_loop_contents();
        }
        capture_started = time_us_64();
        camera_pipeline_capture_to(g_frame_storage[capture_index]);
#endif
    }
}
