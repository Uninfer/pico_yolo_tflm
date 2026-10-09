#include "ov5640_camera.h"

#include "board_pins.h"
#include "camera_capture.h"
#include "hardware/gpio.h"
#include "ov5640.h"

static bool wait_for_pin_level(uint32_t pin, bool level, uint64_t deadline_us)
{
    while (gpio_get(pin) != level) {
        if (time_us_64() >= deadline_us) return false;
        tight_loop_contents();
    }
    return true;
}

static void camera_configure_anti_banding(void)
{
    const uint32_t vsync_pin = BOARD_CAMERA_PIN_VSYNC;
    const uint8_t address = 0x3c;
    const uint32_t samples = 4U;
    const uint64_t timeout = time_us_64() + 500000U;

    if (!wait_for_pin_level(vsync_pin, false, timeout) ||
        !wait_for_pin_level(vsync_pin, true, timeout)) {
        printf("Camera anti-banding: VSYNC timeout, keeping defaults\n");
        return;
    }

    const uint64_t started = time_us_64();
    for (uint32_t i = 0U; i < samples; ++i) {
        const uint64_t deadline = time_us_64() + 200000U;
        if (!wait_for_pin_level(vsync_pin, false, deadline) ||
            !wait_for_pin_level(vsync_pin, true, deadline)) {
            printf("Camera anti-banding: VSYNC measurement failed\n");
            return;
        }
    }

    const uint32_t period_us = (uint32_t)((time_us_64() - started) / samples);
    const uint16_t vts = (uint16_t)(((uint16_t)OV5640_RD_Reg(i2c1, address, 0x380e) << 8U) |
                                    OV5640_RD_Reg(i2c1, address, 0x380f));
    /* LED/mains light has two brightness peaks per electrical cycle:
       10 ms at 50 Hz and 8.333 ms at 60 Hz. */
    uint32_t step50 = (10000U * vts + period_us / 2U) / period_us;
    uint32_t step60 = (25000U * vts + (period_us * 3U) / 2U) /
                      (period_us * 3U);
    if (step50 < 1U) step50 = 1U;
    if (step50 > 1023U) step50 = 1023U;
    if (step60 < 1U) step60 = 1U;
    if (step60 > 1023U) step60 = 1023U;

    /* 夜间曝光最多延长到约两个基础帧周期。
       保持行时钟不变，所以防频闪步长不变，只增加允许的曝光步数。 */
    const uint32_t max_exposure_lines = 2U * vts - 4U;
    uint32_t max50 = max_exposure_lines / step50;
    uint32_t max60 = max_exposure_lines / step60;
    if (max50 > 255U) max50 = 255U;
    if (max60 > 255U) max60 = 255U;

    OV5640_WR_Reg(i2c1, address, 0x3a08, (uint8_t)(step50 >> 8U));
    OV5640_WR_Reg(i2c1, address, 0x3a09, (uint8_t)step50);
    OV5640_WR_Reg(i2c1, address, 0x3a0a, (uint8_t)(step60 >> 8U));
    OV5640_WR_Reg(i2c1, address, 0x3a0b, (uint8_t)step60);
    OV5640_WR_Reg(i2c1, address, 0x3a0e, (uint8_t)max50);
    OV5640_WR_Reg(i2c1, address, 0x3a0d, (uint8_t)max60);
    OV5640_WR_Reg(i2c1, address, 0x3a00,
                  OV5640_RD_Reg(i2c1, address, 0x3a00) | 0x20U);
    /* The board is deployed on 50 Hz mains; avoid unstable auto-detection
       under LED lighting with a weak flicker component. */
    OV5640_WR_Reg(i2c1, address, 0x3c01, 0xa4);
    OV5640_WR_Reg(i2c1, address, 0x3c00, 0x04);

    /* 先设置防频闪，再允许暗光延长曝光；上限取防频闪步长的整数倍。 */
    const uint32_t exposure50 = max50 * step50;
    const uint32_t exposure60 = max60 * step60;
    OV5640_WR_Reg(i2c1, address, 0x3a02, (uint8_t)(exposure60 >> 8U));
    OV5640_WR_Reg(i2c1, address, 0x3a03, (uint8_t)exposure60);
    OV5640_WR_Reg(i2c1, address, 0x3a14, (uint8_t)(exposure50 >> 8U));
    OV5640_WR_Reg(i2c1, address, 0x3a15, (uint8_t)exposure50);
    OV5640_WR_Reg(i2c1, address, 0x3a17, 0x02);
    OV5640_WR_Reg(i2c1, address, 0x3a00,
                  OV5640_RD_Reg(i2c1, address, 0x3a00) | 0x24U);
    const uint16_t read_exposure50 =
        ((uint16_t)OV5640_RD_Reg(i2c1, address, 0x3a14) << 8U) |
        OV5640_RD_Reg(i2c1, address, 0x3a15);
    const uint16_t read_exposure60 =
        ((uint16_t)OV5640_RD_Reg(i2c1, address, 0x3a02) << 8U) |
        OV5640_RD_Reg(i2c1, address, 0x3a03);
    const bool quality_ok =
        OV5640_RD_Reg(i2c1, address, 0x3a18) == 0 &&
        OV5640_RD_Reg(i2c1, address, 0x3a19) == 0x60 &&
        OV5640_RD_Reg(i2c1, address, 0x5306) == 0x10 &&
        (OV5640_RD_Reg(i2c1, address, 0x5308) & 0x50U) == 0x50U &&
        (OV5640_RD_Reg(i2c1, address, 0x3a00) & 0x24U) == 0x24U &&
        read_exposure50 == exposure50 && read_exposure60 == exposure60;
    printf("%s: camera quality register readback\n", quality_ok ? "PASS" : "FAIL");
    printf("Camera exposure limits: 50Hz=%u lines 60Hz=%u lines\n",
           read_exposure50, read_exposure60);

    printf("Camera quality: gain_limit=%u/16 dns=%u cip=0x%02x night=0x%02x max_exp=%lu lines\n",
           (unsigned)OV5640_RD_Reg(i2c1, address, 0x3a19),
           (unsigned)OV5640_RD_Reg(i2c1, address, 0x5306),
           (unsigned)OV5640_RD_Reg(i2c1, address, 0x5308),
           (unsigned)OV5640_RD_Reg(i2c1, address, 0x3a00),
           (unsigned long)max_exposure_lines);

    printf("Camera sensor=%lu.%lu fps anti-band=50Hz step=%lu max=%lu (60Hz %lu/%lu)\n",
           (unsigned long)(10000000U / period_us / 10U),
           (unsigned long)(10000000U / period_us % 10U),
           (unsigned long)step50, (unsigned long)max50,
           (unsigned long)step60, (unsigned long)max60);
}

void ov5640_camera_init(uint16_t *framebuffer)
{
    // PWDN is active high; drive it low to enable the camera module.
    gpio_init(BOARD_CAMERA_PIN_PWDN);
    gpio_set_dir(BOARD_CAMERA_PIN_PWDN, GPIO_OUT);
    gpio_put(BOARD_CAMERA_PIN_PWDN, 0);

    camera_capture_init(framebuffer);
    camera_capture_configure_dma();
    camera_capture_start_pio();
    camera_configure_anti_banding();
}

void ov5640_camera_request_frame(void)
{
    camera_capture_next_frame();
}

bool ov5640_camera_frame_ready(void)
{
    return camera_capture_frame_ready();
}

void ov5640_camera_ack_frame(void)
{
    camera_capture_ack_frame();
}

void ov5640_camera_capture_to(uint16_t *framebuffer)
{
    camera_capture_to(framebuffer);
}

void ov5640_camera_read_exposure(uint16_t *gain_x16, uint32_t *exposure_x16)
{
    const uint8_t address = 0x3c;
    *gain_x16 = (uint16_t)(((uint16_t)(OV5640_RD_Reg(i2c1, address, 0x350a) & 0x03U) << 8U) |
                           OV5640_RD_Reg(i2c1, address, 0x350b));
    *exposure_x16 = ((uint32_t)(OV5640_RD_Reg(i2c1, address, 0x3500) & 0x0fU) << 16U) |
                    ((uint32_t)OV5640_RD_Reg(i2c1, address, 0x3501) << 8U) |
                    OV5640_RD_Reg(i2c1, address, 0x3502);
}
