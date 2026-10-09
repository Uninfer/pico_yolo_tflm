/*****************************************************************************
* | File      	:   cam.c
* | Author      :   Waveshare team
* | Function    :   CAM function interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-03-13
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "camera_capture.h"
#include "board_pins.h"
#include "ov5640.h"
#include "camera_capture.pio.h"

// init PIO
static PIO pio_cam = pio0;

// statemachine's pointer
static const uint32_t sm_cam = 0;

// dma channels
static uint32_t DMA_CAM_RD_CH;

// private functions and buffers
static uint16_t *cam_ptr;

static volatile bool frame_ready = false;

static void camera_capture_dma_irq(void);
static dma_channel_config camera_capture_dma_config(PIO pio, uint32_t sm, uint32_t dma_chan);
static void set_pwm_freq_kHz(uint32_t freq_khz, uint8_t gpio_num);

/********************************************************************************
function:   Camera initialization
parameter:
********************************************************************************/
void camera_capture_init(uint16_t *framebuffer)
{
    cam_ptr = framebuffer;

    // OV5640 XCLK is GP11; the sensor's SCCB bus is on GP22/GP23.
    set_pwm_freq_kHz(24000, BOARD_CAMERA_PIN_XCLK);
    sleep_ms(50);
    ov5640_sensor_init(BOARD_CAMERA_SCCB_SDA_PIN, BOARD_CAMERA_SCCB_SCL_PIN);
    sleep_ms(50);
}

/********************************************************************************
function:   Configuring DMA
parameter:
********************************************************************************/
void camera_capture_configure_dma(void)
{
    // init DMA
    DMA_CAM_RD_CH = dma_claim_unused_channel(true);
    printf("DMA_CH= %d\n", DMA_CAM_RD_CH);

    // Disable IRQ
    irq_set_enabled(DMA_IRQ_0, false);

    // Configure DMA Channel 0
    dma_channel_config c0 = camera_capture_dma_config(pio_cam, sm_cam, DMA_CAM_RD_CH);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_16);

    dma_channel_configure(DMA_CAM_RD_CH, &c0,
                          cam_ptr,               // Destination pointer
                          &pio_cam->rxf[sm_cam], // Source pointer
                          BOARD_LCD_WIDTH * BOARD_LCD_HEIGHT, // pixels per frame
                          false                  // Don't Start yet
    );

    // IRQ settings
    dma_channel_set_irq0_enabled(DMA_CAM_RD_CH, true);
    irq_set_exclusive_handler(DMA_IRQ_0, camera_capture_dma_irq);
    irq_set_enabled(DMA_IRQ_0, true);
}

/********************************************************************************
function:   Start the camera
parameter:
********************************************************************************/
void camera_capture_start_pio(void)
{
    uint32_t offset_cam = pio_add_program(pio_cam, &camera_capture_program);
    // GP0-GP7 carry D0-D7; GP8-GP10 carry VSYNC, HREF and PCLK.
    camera_capture_program_init(pio_cam, sm_cam, offset_cam,
                                BOARD_CAMERA_DATA_BASE_PIN, 11);

    // Enable the state machine and clear the FIFO
    pio_sm_set_enabled(pio_cam, sm_cam, false);
    pio_sm_clear_fifos(pio_cam, sm_cam);
    pio_sm_restart(pio_cam, sm_cam);
    pio_sm_set_enabled(pio_cam, sm_cam, true);

}

/********************************************************************************
function:   Reading camera data
parameter:
********************************************************************************/
/********************************************************************************
function:   DMA interrupt processing function
parameter:
********************************************************************************/
static void camera_capture_dma_irq(void)
{
    uint32_t triggered_dma = dma_hw->ints0;

    if (triggered_dma & (1u << DMA_CAM_RD_CH))
    {
        // Clear interrupt flag
        dma_hw->ints0 = 1u << DMA_CAM_RD_CH;
        __compiler_memory_barrier();
        frame_ready = true;
    }

}

void camera_capture_to(uint16_t *framebuffer)
{
    cam_ptr = framebuffer;
    const uint32_t transfer_count = BOARD_LCD_WIDTH * BOARD_LCD_HEIGHT;
    const uint32_t channel_mask = 1u << DMA_CAM_RD_CH;

    // Match the reference driver's frame transaction: start a fresh DMA
    // transfer, then give the PIO program both words it consumes at start0.
    dma_hw->ints0 = channel_mask;
    frame_ready = false;
    __compiler_memory_barrier();
    dma_channel_set_write_addr(DMA_CAM_RD_CH, cam_ptr, false);
    dma_channel_set_trans_count(DMA_CAM_RD_CH, transfer_count, false);
    dma_start_channel_mask(channel_mask);

    // PIO starts with OUT X, OUT Y: X is initialized to zero, Y is the total
    // pixel count minus one. It then captures exactly one VSYNC-bounded frame.
    pio_sm_put_blocking(pio_cam, sm_cam, 0);
    pio_sm_put_blocking(pio_cam, sm_cam, transfer_count - 1u);
}

bool camera_capture_frame_ready(void)
{
    return frame_ready;
}

void camera_capture_ack_frame(void)
{
    frame_ready = false;
}

/********************************************************************************
function:   Release the camera
parameter:
********************************************************************************/
void camera_capture_deinit(void)
{
    // Disable IRQ settings
    irq_set_enabled(DMA_IRQ_0, false);
    dma_channel_set_irq0_enabled(DMA_CAM_RD_CH, false);
    dma_channel_abort(DMA_CAM_RD_CH);
}

/********************************************************************************
function:   Camera dma config
parameter:
********************************************************************************/
static dma_channel_config camera_capture_dma_config(PIO pio, uint32_t sm, uint32_t dma_chan)
{
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
    return c;
}

/********************************************************************************
function:   Configure PWM to provide clock for the camera
parameter:
********************************************************************************/
static void set_pwm_freq_kHz(uint32_t freq_khz, uint8_t gpio_num)
{
    uint32_t pwm0_slice_num;
    uint32_t period;
    static pwm_config pwm_slice_config;
    uint32_t system_clk_khz = clock_get_hz(clk_sys) / 1000;
    period = system_clk_khz / freq_khz - 1;
    if (period < 1)
        period = 1;

    gpio_set_function(gpio_num, GPIO_FUNC_PWM);
    pwm0_slice_num = pwm_gpio_to_slice_num(gpio_num);

    // config
    pwm_slice_config = pwm_get_default_config();
    pwm_config_set_wrap(&pwm_slice_config, period);

    // set clk div
    pwm_config_set_clkdiv(&pwm_slice_config, 1);

    // set PWM start
    pwm_init(pwm0_slice_num, &pwm_slice_config, true);
    pwm_set_gpio_level(gpio_num, (pwm_slice_config.top * 0.50)); // duty:50%
}

void camera_capture_next_frame(void)
{
    camera_capture_to(cam_ptr);
}
