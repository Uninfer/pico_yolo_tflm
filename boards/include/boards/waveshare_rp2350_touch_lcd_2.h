/*
 * Board definition for the Waveshare RP2350-Touch-LCD-2 / 2-C.
 * The -C revision is supplied with an OV5640 camera module.
 */

// pico_cmake_set PICO_PLATFORM=rp2350
// pico_cmake_set_default PICO_FLASH_SIZE_BYTES = (16 * 1024 * 1024)

#ifndef _BOARDS_WAVESHARE_RP2350_TOUCH_LCD_2_H
#define _BOARDS_WAVESHARE_RP2350_TOUCH_LCD_2_H

#define PICO_RP2350A 1

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

// Display SPI bus: GP18 SCK, GP19 MOSI, GP17 CS. MISO is unused.
#define PICO_DEFAULT_SPI 0
#define PICO_DEFAULT_SPI_SCK_PIN 18
#define PICO_DEFAULT_SPI_TX_PIN 19
#define PICO_DEFAULT_SPI_RX_PIN 16
#define PICO_DEFAULT_SPI_CSN_PIN 17

// Shared touch/IMU I2C bus.
#define PICO_DEFAULT_I2C 0
#define PICO_DEFAULT_I2C_SDA_PIN 12
#define PICO_DEFAULT_I2C_SCL_PIN 13

// This board has no user LED. GP25 is the microSD chip-select pin.

#endif
