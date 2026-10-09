/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "st7789.h"

static struct st7789_config st7789_cfg;
static uint16_t st7789_width;
static uint16_t st7789_height;
static bool st7789_data_mode = false;

static void st7789_cmd(uint8_t cmd, const uint8_t* data, size_t len)
{
    if (st7789_cfg.gpio_cs > -1) {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    } else {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }
    st7789_data_mode = false;

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 0);
    sleep_us(1);

    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));

    if (len) {
        sleep_us(1);
        gpio_put(st7789_cfg.gpio_dc, 1);
        sleep_us(1);

        spi_write_blocking(st7789_cfg.spi, data, len);
    }

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 1);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    sleep_us(1);
}

static void st7789_caset(uint16_t xs, uint16_t xe)
{
    uint8_t data[] = {
        (uint8_t)(xs >> 8),
        (uint8_t)(xs & 0xff),
        (uint8_t)(xe >> 8),
        (uint8_t)(xe & 0xff),
    };

    // CASET (2Ah): Column Address Set
    st7789_cmd(0x2a, data, sizeof(data));
}

static void st7789_raset(uint16_t ys, uint16_t ye)
{
    uint8_t data[] = {
        (uint8_t)(ys >> 8),
        (uint8_t)(ys & 0xff),
        (uint8_t)(ye >> 8),
        (uint8_t)(ye & 0xff),
    };

    // RASET (2Bh): Row Address Set
    st7789_cmd(0x2b, data, sizeof(data));
}


void st7789_init(const struct st7789_config* config, uint16_t width, uint16_t height)
{
    memcpy(&st7789_cfg, config, sizeof(st7789_cfg));
    st7789_width = width;
    st7789_height = height;

    /* Match the known-good reference project request. At a 300 MHz peri clock
       the PL022 divider selects the highest rate not exceeding 125 MHz. */
    const uint32_t spi_baud = spi_init(st7789_cfg.spi, 125 * 1000 * 1000);
    printf("LCD SPI: %lu Hz\n", (unsigned long)spi_baud);
    if (st7789_cfg.gpio_cs > -1) {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    } else {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }

    gpio_set_function(st7789_cfg.gpio_din, GPIO_FUNC_SPI);
    gpio_set_function(st7789_cfg.gpio_clk, GPIO_FUNC_SPI);

    if (st7789_cfg.gpio_cs > -1) {
        gpio_init(st7789_cfg.gpio_cs);
    }
    gpio_init(st7789_cfg.gpio_dc);
    gpio_init(st7789_cfg.gpio_rst);
    gpio_init(st7789_cfg.gpio_bl);

    if (st7789_cfg.gpio_cs > -1) {
        gpio_set_dir(st7789_cfg.gpio_cs, GPIO_OUT);
    }
    gpio_set_dir(st7789_cfg.gpio_dc, GPIO_OUT);
    gpio_set_dir(st7789_cfg.gpio_rst, GPIO_OUT);
    gpio_set_dir(st7789_cfg.gpio_bl, GPIO_OUT);

    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 1);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    gpio_put(st7789_cfg.gpio_rst, 1);
    sleep_ms(100);

    // SWRESET (01h): Software Reset
    st7789_cmd(0x01, NULL, 0);
    sleep_ms(150);

    // SLPOUT (11h): Sleep Out
    st7789_cmd(0x11, NULL, 0);
    sleep_ms(50);

    // COLMOD (3Ah): Interface Pixel Format
    // - RGB interface color format     = 65K of RGB interface
    // - Control interface color format = 16bit/pixel
    st7789_cmd(0x3a, (uint8_t[]){ 0x55 }, 1);
    sleep_ms(10);

    // MADCTL (36h): Memory Data Access Control
    // - Page Address Order            = Top to Bottom
    // - Column Address Order          = Left to Right
    // - Page/Column Order             = Normal Mode
    // - Line Address Order            = LCD Refresh Top to Bottom
    // - RGB/BGR Order                 = RGB
    // - Display Data Latch Data Order = LCD Refresh Left to Right

    st7789_cmd(0x11, NULL, 0);
    st7789_cmd(0x36, (uint8_t[]){ 0x40 }, 1);   // 240x320 portrait

    st7789_caset(0, width - 1U);
    st7789_raset(0, height - 1U);

    // INVON (21h): Display Inversion On
    st7789_cmd(0x21, NULL, 0);
    sleep_ms(10);

    // NORON (13h): Normal Display Mode On
    st7789_cmd(0x13, NULL, 0);
    sleep_ms(10);

    // DISPON (29h): Display On
    st7789_cmd(0x29, NULL, 0);
    sleep_ms(10);

    gpio_put(st7789_cfg.gpio_bl, 1);
}

static void st7789_ramwr(void)
{
    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 0);
    sleep_us(1);

    // RAMWR (2Ch): Memory Write
    uint8_t cmd = 0x2c;
    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1) {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    sleep_us(1);
}

void st7789_write(const void* data, size_t len)
{
    if (!st7789_data_mode) {
        st7789_ramwr();
        if (st7789_cfg.gpio_cs > -1) {
            spi_set_format(st7789_cfg.spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        } else {
            spi_set_format(st7789_cfg.spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        }

        st7789_data_mode = true;
    }
    spi_write16_blocking(st7789_cfg.spi, (uint16_t *)data, len / 2);
}

void st7789_fill(uint16_t pixel)
{
    uint16_t row[LCD_WIDTH];
    uint16_t y;
    for (y = 0U; y < st7789_width; ++y) row[y] = pixel;
    st7789_set_cursor(0U, 0U);
    for (y = 0U; y < st7789_height; ++y) {
        st7789_write(row, st7789_width * sizeof(row[0]));
    }
}

void st7789_set_cursor(uint16_t x, uint16_t y)
{
    st7789_caset(x, st7789_width - 1U);
    st7789_raset(y, st7789_height - 1U);
}

static void st7789_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    st7789_caset(x1, x2);
    st7789_raset(y1, y2);
}

void st7789_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                      uint16_t color)
{
    uint16_t span[(LCD_WIDTH > LCD_HEIGHT) ? LCD_WIDTH : LCD_HEIGHT];
    uint16_t index;
    const uint16_t width = x2 - x1 + 1U;
    const uint16_t height = y2 - y1 + 1U;
    const uint16_t maximum = width > height ? width : height;
    for (index = 0U; index < maximum; ++index) span[index] = color;

    st7789_set_window(x1, y1, x2, y1);
    st7789_write(span, width * sizeof(span[0]));
    if (y2 != y1) {
        st7789_set_window(x1, y2, x2, y2);
        st7789_write(span, width * sizeof(span[0]));
    }
    st7789_set_window(x1, y1, x1, y2);
    st7789_write(span, height * sizeof(span[0]));
    if (x2 != x1) {
        st7789_set_window(x2, y1, x2, y2);
        st7789_write(span, height * sizeof(span[0]));
    }
}
