#include "ov5640_camera.h"

#include "board_pins.h"
#include "camera_capture.h"
#include "hardware/gpio.h"

void ov5640_camera_init(uint16_t *framebuffer)
{
    // PWDN is active high; drive it low to enable the camera module.
    gpio_init(BOARD_CAMERA_PIN_PWDN);
    gpio_set_dir(BOARD_CAMERA_PIN_PWDN, GPIO_OUT);
    gpio_put(BOARD_CAMERA_PIN_PWDN, 0);

    camera_capture_init(framebuffer);
    camera_capture_configure_dma();
    camera_capture_start_pio();
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
