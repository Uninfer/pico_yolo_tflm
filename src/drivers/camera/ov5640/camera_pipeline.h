#ifndef CAMERA_PIPELINE_H
#define CAMERA_PIPELINE_H
#include "ov5640_camera.h"
static inline void camera_pipeline_init(uint16_t *frame) { ov5640_camera_init(frame); ov5640_camera_capture_to(frame); }
static inline bool camera_pipeline_frame_ready(void) { return ov5640_camera_frame_ready(); }
static inline void camera_pipeline_capture_to(uint16_t *frame) { ov5640_camera_capture_to(frame); }
static inline void camera_pipeline_read_exposure(uint16_t *gain, uint32_t *exposure) { ov5640_camera_read_exposure(gain, exposure); }
#endif
