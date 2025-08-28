#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/spi.h"

#include "main_functions.h" // 包含推理相关函数

#include <string.h>
#include <stdlib.h>
#include "hardware/vreg.h" // 包含电压调节头文件
#include "hardware/i2c.h"
#include "hardware/pio.h" // 包含PIO头文件
#include "math.h"

#include "lcd.h"
#include "camera.h"
#include "lcd_config.h"
#include "config.h"

#include "pico/multicore.h" // 多核
#include "pico/sync.h"

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240

#define WHITE          0xFFFF
#define BLACK          0x0000
#define BLUE           0x001F
#define BRED           0XF81F
#define GRED           0XFFE0
#define GBLUE          0X07FF
#define RED            0xF800
#define MAGENTA        0xF81F
#define GREEN          0x07E0
#define CYAN           0x7FFF
#define YELLOW         0xFFE0
#define BROWN          0XBC40
#define BRRED          0XFC07
#define GRAY           0X8430

// 定义颜色（RGB565）
const uint16_t colors[] = {
    0x07E0, // 绿色    (Green)
    0xF800, // 红色    (Red)
    0x001F, // 蓝色    (Blue)
    0xFFE0, // 黄色    (Yellow)
    0xFFFF, // 白色    (White)
};

/* 打印图像数组中的像素值（RGB565, uint16_t），16进制格式，带逗号，每行固定数量 */
void print_decimal_image_uint16(uint16_t *image_buffer, int width, int height)
{
    int total_pixels = width * height;

    for (int i = 0; i < total_pixels; i++) {
        uint16_t pixel = image_buffer[i];
        // 高低字节交换
        pixel = (pixel >> 8) | (pixel << 8);

        printf("0x%04X", pixel); // 打印 16位 0xXXXX
        if (i < total_pixels - 1) {
            printf(", ");
        }
    }
    printf("\n");
}

BBox boxes_cache[MAX_BOXES]; // 缓存上一次推理结果
int box_count_cache = 0;

BBox share_boxes[MAX_BOXES]; // 缓存上一次推理结果
int share_box_count = 0;

mutex_t g_image_buffer_lock;
mutex_t g_box_lock;
extern void dengdai(void);

// framebuffer 放 SRAM0
#define DUMMY_BYTE 10 // 配置 10-12 比较合适
__attribute__((section(".sram0"))) uint8_t frame_ov7670[FRAME_WIDTH * FRAME_HEIGHT * 2 + DUMMY_BYTE];
uint16_t *image_buffer = NULL;

/* 画框 */
void draw_box(void)
{
    mutex_enter_blocking(&g_box_lock);
    for (int i = 0; i < share_box_count; i++) {
        printf(__FILE__ ":%d\n", __LINE__);
        int x1 = (int)(share_boxes[i].x1 * FRAME_WIDTH);
        int y1 = (int)(share_boxes[i].y1 * FRAME_HEIGHT);
        int x2 = (int)(share_boxes[i].x2 * FRAME_WIDTH);
        int y2 = (int)(share_boxes[i].y2 * FRAME_HEIGHT);
        uint16_t color = colors[share_boxes[i].class_id];
        draw_boxes_to_lcd(x1, y1, x2, y2, color);
    }
    mutex_exit(&g_box_lock);
}

void core1_entry()
{
    while (1) {
        /********** 推理 **********/
        printf("Frame captured, processing...\n");
        // 打印图像数据
        // print_decimal_image_uint16(image_buffer, FRAME_WIDTH, FRAME_HEIGHT); // uint16_t 数组

        // pico_yolo_infer(image_buffer, boxes_cache, &box_count_cache);        // 执行推理

        // 判断是否 跳帧 需要推理


        pico_yolo_infer(image_buffer, boxes_cache, &box_count_cache); // 执行推理

        /* 拷贝到共享区域 */
        mutex_enter_blocking(&g_box_lock);
        share_box_count = box_count_cache;
        if (share_box_count > 0) {
            memcpy(share_boxes, boxes_cache, share_box_count * sizeof(BBox));
        }
        mutex_exit(&g_box_lock);
    }
}

#include "GUI_Paint.h"
#include "LCD_2in.h"
#include "DEV_Config.h"
#include "cam.h"

int main(void)
{
//     set_sys_clock_khz(300000, true);
//     vreg_set_voltage(VREG_VOLTAGE_MAX);
//     stdio_init_all();
//     mutex_init(&g_image_buffer_lock);                       //  初始化锁
//     mutex_init(&g_box_lock);                                //  初始化锁
//     printf("System clock set to %lu Hz\r\n", clock_get_hz(clk_sys));

//     pico_yolo_init();                                       // 初始化推理模型
//     lcd_init();                                             // 初始化 LCD
//     camera_init();                                          // 初始化摄像头
//     multicore_launch_core1(core1_entry);                    // 启动从核
//     while (true) {
//         /*************************** 获取 一帧数据  ***************************/
//         mutex_enter_blocking(&g_image_buffer_lock);
//         camera_fill_frame();
//         // draw_box();                                         /* 画框 */
//         camera_wait_for_dma_ok();
//         camera_swap_bytes();
//         mutex_exit(&g_image_buffer_lock);

//         /**************************** LCD显示图片 ****************************/
// #ifdef CONFIG_LCD_USE_DMA
//         lcd_show_image_dma(image_buffer);
// #else
//         lcd_show_image(image_buffer);
// #endif
//         draw_box();                                         /* 画框 */
//     }
//     return 0;
    if(DEV_Module_Init()!=0){
        return -1;
    }
    DEV_SET_PWM(0);
    // image_buffer = (uint16_t *)(frame_ov7670 + DUMMY_BYTE); // RGB565 uint16数组
    UDOUBLE Imagesize = LCD_2IN_HEIGHT*LCD_2IN_WIDTH*2;
    if((image_buffer = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }

    mutex_init(&g_image_buffer_lock);                       //  初始化锁
    mutex_init(&g_box_lock);                                //  初始化锁
    printf("System clock set to %lu Hz\r\n", clock_get_hz(clk_sys));

    pico_yolo_init();                                       // 初始化推理模型
    lcd_init();
    multicore_launch_core1(core1_entry);                    // 启动从核


    // TODO 第一帧比较特殊, 后面修改
    mutex_enter_blocking(&g_image_buffer_lock);
    camera_init();
    while (buffer_ready == false) { // 等待 DMA 完成
        ;
    }
    camera_swap_bytes();
    mutex_exit(&g_image_buffer_lock);
    mutex_exit(&g_image_buffer_lock);
    lcd_show_image(image_buffer);
    draw_box();                                     /* 画框 */

    while(1) {
        mutex_enter_blocking(&g_image_buffer_lock);
        camera_fill_frame();
        while (buffer_ready == false) { // 等待 DMA 完成
            ;
        }
        buffer_ready = false;  // Reset the flag
        camera_swap_bytes();
        mutex_exit(&g_image_buffer_lock);
        lcd_show_image(image_buffer);
        draw_box();                                     /* 画框 */
    }
}
