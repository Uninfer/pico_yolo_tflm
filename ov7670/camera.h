
#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void camera_init(void);
void camera_fill_frame(void);
void camera_wait_for_dma_ok(void);
void camera_swap_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
