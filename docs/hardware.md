# Hardware

RP2350, an OV5640 DVP camera, and a 240×320 ST7789 SPI display. This project targets the Waveshare RP2350-Touch-LCD-2-C board with the following peripheral pins.

| GPIO | Signal |
| --- | --- |
| GP0–GP7 | Camera D0–D7 |
| GP8 / GP9 / GP10 | VSYNC / HREF / PCLK |
| GP11 | Camera XCLK (PWM; requested 24 MHz, actual frequency quantized by the system clock) |
| GP21 | Camera PWDN (active-high power-down) |
| GP22 / GP23 | SCCB SDA / SCL (I²C1) |
| GP15 / GP16 / GP17 | LCD BL / DC / CS |
| GP18 / GP19 / GP20 | LCD SCK / MOSI / RST (SPI0) |

Check the module supply voltage and connect a common ground when wiring. Board documentation is available on the [Waveshare Wiki](https://www.waveshare.net/wiki/RP2350-Touch-LCD-2).

## Current camera and display settings

The camera outputs 240×320 RGB565. At 300 MHz, the base sensor output is approximately 29.4 FPS, the actual LCD SPI clock is approximately 75 MHz, and a full-frame transfer takes approximately 18 ms. The firmware retains the previously tested camera settings: a 6× automatic gain limit, manual denoising threshold of 16, reduced sharpening, night-mode automatic exposure, and 50 Hz anti-banding. Subsequent experiments in very dark conditions were reverted.

Startup logs read back camera settings. These settings were tested mainly under dim indoor LED lighting. Noise, brightness fluctuations, or horizontal banding may still occur in very dark conditions; complete elimination has not been verified. Actual exposure time cannot be inferred from the base frame rate alone.

## Troubleshooting

| Symptom | Checks |
| --- | --- |
| Pico SDK not found | Set `PICO_SDK_PATH` and initialize SDK submodules |
| Cross-compiler not found | Add the Arm GNU Toolchain `bin` directory to PATH |
| RP2350 boot drive does not appear | Reconnect USB while holding BOOT/BOOTSEL and check the data cable |
| Black display or no camera image | Check pins, PWDN, power, and SCCB initialization logs |
| Colored noise, horizontal banding, or flicker in dim light | Check wire length, power supply, and lighting PWM; try steady supplemental lighting |
| Unexpected resets | Check power and retest at the 150 MHz baseline clock |

Before changing driver settings, record the original values and an image of the same scene, then compare image quality, throughput, and latency.
