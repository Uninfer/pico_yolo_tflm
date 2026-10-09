#ifndef PICO_YOLO_WX_CAMERA_CAPTURE_H
#define PICO_YOLO_WX_CAMERA_CAPTURE_H

#include <stdbool.h>
#include <stdint.h>

void camera_capture_init(uint16_t *framebuffer);
void camera_capture_configure_dma(void);
void camera_capture_start_pio(void);
void camera_capture_next_frame(void);
void camera_capture_to(uint16_t *framebuffer);
void camera_capture_deinit(void);
bool camera_capture_frame_ready(void);
void camera_capture_ack_frame(void);

#endif
