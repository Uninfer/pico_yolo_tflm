#include "lcd_config.h"
#include "lcd.h"
#include "st7789.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"

// LCD configuration
static const struct st7789_config lcd_config = {
    .spi = spi0,
    .gpio_din = BOARD_LCD_PIN_MOSI,
    .gpio_clk = BOARD_LCD_PIN_SCK,
    .gpio_cs = BOARD_LCD_PIN_CS,
    .gpio_dc = BOARD_LCD_PIN_DC,
    .gpio_rst = BOARD_LCD_PIN_RESET,
    .gpio_bl = BOARD_LCD_PIN_BL,
};

/**
 * @brief   lcd  初始化
 *
 */
void lcd_init(void)
{
    st7789_init(&lcd_config, CONFIG_LCD_WIDTH, CONFIG_LCD_HEIGHT);
    st7789_dma_init();
    st7789_fill(0x0000);                // 清屏
}

/**
 * @brief   显示一张完成的图像
 *
 * @param   image           uint16_t 类型的数组
 */
void lcd_show_image(uint16_t *image)
{
    st7789_set_cursor(0, 0);
    st7789_write(image, CONFIG_IMAGE_SIZE);
}

void lcd_show_image_dma(uint16_t *image)
{
    st7789_draw_image_dma(image);                           // 使用 DMA 传输图像数据, 但是!!!速度并没有快
}

void lcd_wait_for_dma_ok(void)
{
    st7789_wait_for_dma_ok();
}

extern uint16_t *image_buffer;
#define FRAME_WIDTH CONFIG_LCD_WIDTH
#define FRAME_HEIGHT CONFIG_LCD_HEIGHT
void draw_boxes_to_lcd(int x1, int y1, int x2, int y2, uint16_t color)
{
    if (x2 < 0 || y2 < 0 || x1 >= FRAME_WIDTH || y1 >= FRAME_HEIGHT) {
        return;
    }
    /* 限制范围 */
    if (x1 < 0) {
        x1 = 0;
    }
    if (x2 >= FRAME_WIDTH) {
        x2 = FRAME_WIDTH - 1;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (y2 >= FRAME_HEIGHT) {
        y2 = FRAME_HEIGHT - 1;
    }
    if (x1 > x2 || y1 > y2) {
        return;
    }

    st7789_fill_region((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y1, color);
    if (y2 != y1) {
        st7789_fill_region((uint16_t)x1, (uint16_t)y2, (uint16_t)x2, (uint16_t)y2, color);
    }
    if (y2 - y1 > 1) {
        st7789_fill_region((uint16_t)x1, (uint16_t)(y1 + 1), (uint16_t)x1, (uint16_t)(y2 - 1), color);
        if (x2 != x1) {
            st7789_fill_region((uint16_t)x2, (uint16_t)(y1 + 1), (uint16_t)x2, (uint16_t)(y2 - 1), color);
        }
    }
}

// image_buffer[320 * y1 + x] = color;
// image_buffer[320 * y2+ x] = color;
// image_buffer[320 * y+ x1] = color;
// image_buffer[320 * y+ x2] = color;
