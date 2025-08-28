#include "camera.h"

#include <stdio.h>
// #include "ov7670.h"
#include "hardware/pio.h" // 包含PIO头文件

#define FRAME_WIDTH     320
#define FRAME_HEIGHT    240
#define PIXELS_SIZE     76800   // 像素个数  240 * 320
#define FRAME_RATE 30


const int PIN_CAM_SIOC = 21; // I2C0 SCL
const int PIN_CAM_SIOD = 4;  // I2C0 SDA
const int PIN_CAM_RESETB = 17;
const int PIN_CAM_XCLK = 3;
const int PIN_CAM_VSYNC = 16;
const int PIN_CAM_Y2_PIO_BASE = 6;

#include "cam.h"

extern uint16_t *image_buffer;

// struct ov7670_config camera_config;

/**
 * @brief   初始化摄像头
 *
 */
void camera_init(void)
{

    init_cam();          // initialize camera
    config_cam_buffer(); // config buffer
    start_cam();         // start streaming

    // camera_config.sccb = i2c0;
    // camera_config.pin_sioc = PIN_CAM_SIOC;
    // camera_config.pin_siod = PIN_CAM_SIOD;
    // camera_config.pin_resetb = PIN_CAM_RESETB;
    // camera_config.pin_xclk = PIN_CAM_XCLK;
    // camera_config.pin_vsync = PIN_CAM_VSYNC;
    // camera_config.pin_y2_pio_base = PIN_CAM_Y2_PIO_BASE;
    // camera_config.pio = pio0;
    // camera_config.pio_sm = 0;
    // camera_config.dma_channel = 0;
    // camera_config.image_buf = frame_ov7670;
    // camera_config.image_buf_size = sizeof(frame_ov7670);
    // ov7670_init(&camera_config);
    // // uint8_t midh = ov7670_reg_read(&camera_config, 0x1C);
    // // uint8_t midl = ov7670_reg_read(&camera_config, 0x1D);
    // // printf("OV7670 ID: 0x%02X%02X\n", midh, midl);          // Debug camera ID
    // image_buffer = (uint16_t *)(frame_ov7670 + DUMMY_BYTE); // RGB565 uint16数组
}

/* 高低字节交换 */
void camera_swap_bytes(void)
{
    for (size_t i = 0; i < PIXELS_SIZE; i++) {
        image_buffer[i] = __builtin_bswap16(image_buffer[i]);   // 内建指令大小端转化
    }
}

/**
 * @brief   获取一帧数据
 *
 */
extern void ov5640_capture_frame(void);
void camera_fill_frame(void)
{
    ov5640_capture_frame();
    // ov7670_capture_frame(&camera_config);
    // camera_swap_bytes();            // 交换高低字节
}

void camera_wait_for_dma_ok(void)
{
    // ov7670_wait_for_dma_ok(&camera_config);
}