#include "lcd_config.h"
#include "lcd.h"
// #include "st7789.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/spi.h"

// LCD configuration
// static const struct st7789_config lcd_config = {
//     .spi = spi_default,
//     .gpio_din = PICO_DEFAULT_SPI_TX_PIN,
//     .gpio_clk = PICO_DEFAULT_SPI_SCK_PIN,
//     .gpio_cs = LCD_CS, // 17 摄像头占用
//     .gpio_dc = LCD_DC,
//     .gpio_rst = LCD_RST, // 21 摄像头占用
//     .gpio_bl = LCD_BL,
// };

#include "GUI_Paint.h"
#include "LCD_2in.h"

extern uint16_t *image_buffer;

/**
 * @brief   lcd  初始化
 *
 */
void lcd_init(void)
{
    /* LCD Init */
    printf("2inch LCD demo...\r\n");
    LCD_2IN_Init(VERTICAL);
    LCD_2IN_Clear(WHITE);
    DEV_SET_PWM(100);

    /* Create a new image cache named IMAGE_RGB and fill it with white */
    Paint_NewImage((UBYTE *)image_buffer, LCD_2IN.WIDTH, LCD_2IN.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_Clear(WHITE);
    sleep_ms(10);
//     st7789_init(&lcd_config, 320, 240); // 初始化 LCD
//     st7789_dma_init();
//     st7789_fill(0x0000);                // 清屏
}

/**
 * @brief   显示一张完成的图像
 *
 * @param   image           uint16_t 类型的数组
 */
void lcd_show_image(uint16_t *image)
{
    LCD_2IN_Display((UBYTE *)image);
    // st7789_set_cursor(0, 0);
    // st7789_write(image, CONFIG_IMAGE_SIZE);
}

void lcd_show_image_dma(uint16_t *image)
{
    // st7789_draw_image_dma(image);                           // 使用 DMA 传输图像数据, 但是!!!速度并没有快
}

void lcd_wait_for_dma_ok(void)
{
    // st7789_wait_for_dma_ok();
}

extern uint16_t *image_buffer;
#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
void draw_boxes_to_lcd(int x1, int y1, int x2, int y2, uint16_t color)
{
    // /* 限制范围 */
    // if (x1 < 0) {
    //     x1 = 0;
    // }
    // if (x2 >= FRAME_WIDTH) {
    //     x2 = FRAME_WIDTH - 1;
    // }
    // if (y1 < 0) {
    //     y1 = 0;
    // }
    // if (y2 >= FRAME_HEIGHT) {
    //     y2 = FRAME_HEIGHT - 1;
    // }
    // // // 上边框
    // // st7789_set_cursor(x1, y1);
    // // for (int x = x1; x <= x2; x++) {
    // //     st7789_put(color);
    // // }
    // // // 下边框
    // // if (y2 > y1) {  // 避免重复画单行
    // //     st7789_set_cursor(x1, y2);
    // //     for (int x = x1; x <= x2; x++) {
    // //         st7789_put(color);
    // //     }
    // // }
    // // // 左右边框
    // // for (int y = y1+1; y < y2; y++) { // +1 和 <y2 避免重复角点
    // //     st7789_set_cursor(x1, y);
    // //     st7789_put(color);
    // //     if (x2 > x1) {
    // //         st7789_set_cursor(x2, y);
    // //         st7789_put(color);
    // //     }
    // // }

    /* 上边框 */
    for (int x = x1; x <= x2; x++) {
        LCD_2IN_DisplayPoint(x, y1, color);
    }
    /* 下边框 */
    for (int x = x1; x <= x2; x++) {
        LCD_2IN_DisplayPoint(x, y2, color);
    }
    /* 左边框  */
    for (int y = y1; y <= y2; y++) {
        LCD_2IN_DisplayPoint(x1, y, color);
    }
    /* 右边框 */
    for (int y = y1; y <= y2; y++) {
        LCD_2IN_DisplayPoint(x2, y, color);
    }

    // /* 上边框 */
    // for (int x = x1; x <= x2; x++) {
    //     // st7789_set_cursor(x, y1);
    //     // st7789_put(color);
    //     image_buffer[320 * y1 + x] = color;
    // }
    // /* 下边框 */
    // for (int x = x1; x <= x2; x++) {
    //     // st7789_set_cursor(x, y2);
    //     // st7789_put(color);
    //     image_buffer[320 * y2+ x] = color;
    // }
    // /* 左边框  */
    // for (int y = y1; y <= y2; y++) {
    //     // st7789_set_cursor(x1, y);
    //     // st7789_put(color);
    //     image_buffer[320 * y+ x1] = color;
    // }
    // /* 右边框 */
    // for (int y = y1; y <= y2; y++) {
    //     // st7789_set_cursor(x2, y);
    //     // st7789_put(color);
    //     image_buffer[320 * y+ x2] = color;
    // }
}
