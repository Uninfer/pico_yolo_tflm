#ifndef PICO_YOLO_INFERENCE_H
#define PICO_YOLO_INFERENCE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PICO_YOLO_FRAME_WIDTH 240U
#define PICO_YOLO_FRAME_HEIGHT 320U
#define PICO_YOLO_MAX_BOXES 10U

typedef struct {
    float x1;
    float y1;
    float x2;
    float y2;
    float confidence;
    uint8_t class_id;
} pico_yolo_box_t;

int pico_yolo_init(void);
int pico_yolo_preprocess_frame(const uint16_t *rgb565, uint32_t *elapsed_us);
int pico_yolo_run_preprocessed(pico_yolo_box_t *boxes,
                               uint32_t *box_count,
                               uint32_t *inference_us,
                               uint32_t *postprocess_us);
int pico_yolo_infer(const uint16_t *rgb565,
                    pico_yolo_box_t *boxes,
                    uint32_t *box_count);

/* Release the camera frame after input preparation; the model uses its own input tensor.
 * The wait interface is for adapters that must wait for display reads; standard TFLM does not need it. */
void pico_yolo_wait_for_display(void);
void pico_yolo_release_frame(void);
#ifdef __cplusplus
}
#endif
#endif
