/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _PICO_ST7789_H_
#define _PICO_ST7789_H_

#include "hardware/spi.h"

#define LCD_WIDTH  240  // LCD宽度
#define LCD_HEIGHT 320 // LCD高度

struct st7789_config {
    spi_inst_t* spi;
    uint gpio_din;
    uint gpio_clk;
    int gpio_cs;
    uint gpio_dc;
    uint gpio_rst;
    uint gpio_bl;
};

void st7789_init(const struct st7789_config* config, uint16_t width, uint16_t height);
void st7789_write(const void* data, size_t len);
void st7789_put(uint16_t pixel);
void st7789_fill(uint16_t pixel);
void st7789_set_cursor(uint16_t x, uint16_t y);
void st7789_vertical_scroll(uint16_t row);

void st7789_dma_init();
void st7789_draw_image_dma(const uint16_t *img);
void st7789_wait_for_dma_ok(void);

#endif
