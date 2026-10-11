#include "lcd_config.h"
#include "lcd.h"
#include "st7789.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
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

_Static_assert(CONFIG_LCD_WIDTH == LCD_WIDTH &&
               CONFIG_LCD_HEIGHT == LCD_HEIGHT,
               "LCD dimensions must agree across the driver");

/**
 * @brief   Initialize the LCD
 *
 */
void lcd_init(void)
{
    st7789_init(&lcd_config, CONFIG_LCD_WIDTH, CONFIG_LCD_HEIGHT);
    /* Verify the reliable blocking transfer path before camera display. */
    st7789_fill(0xf800);
    sleep_ms(150);
    st7789_fill(0x07e0);
    sleep_ms(150);
    st7789_fill(0x001f);
    sleep_ms(150);
    st7789_fill(0x0000);
}

/**
 * @brief   Display a complete image
 *
 * @param   image           Array of uint16_t pixels
 */
void lcd_show_image(uint16_t *image)
{
    st7789_set_cursor(0, 0);
    st7789_write(image, CONFIG_IMAGE_SIZE);
}

#define FRAME_WIDTH CONFIG_LCD_WIDTH
#define FRAME_HEIGHT CONFIG_LCD_HEIGHT
void draw_boxes_to_lcd(int x1, int y1, int x2, int y2, uint16_t color)
{
    /* Clamp coordinates to the valid range. */
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
    if (x2 >= x1 && y2 >= y1) {
        st7789_draw_rect((uint16_t)x1, (uint16_t)y1,
                         (uint16_t)x2, (uint16_t)y2, color);
    }
}
