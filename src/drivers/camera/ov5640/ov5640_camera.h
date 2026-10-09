#ifndef PICO_YOLO_WX_OV5640_CAMERA_H
#define PICO_YOLO_WX_OV5640_CAMERA_H

#include <stdbool.h>
#include <stdint.h>

void ov5640_camera_init(uint16_t *framebuffer);
void ov5640_camera_request_frame(void);
void ov5640_camera_capture_to(uint16_t *framebuffer);
void ov5640_camera_read_exposure(uint16_t *gain_x16, uint32_t *exposure_x16);
bool ov5640_camera_frame_ready(void);
void ov5640_camera_ack_frame(void);

#endif
