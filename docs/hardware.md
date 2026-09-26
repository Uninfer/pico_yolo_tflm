# 硬件与引脚

项目唯一目标硬件为 Waveshare [RP2350-Touch-LCD-2-C](https://www.waveshare.net/shop/RP2350-Touch-LCD-2-C.htm) 摄像头套装。固件使用 RP2350A、240 × 320 ST7789T3 屏幕和 OV5640 摄像头。

| GPIO | 信号 | 用途 |
| --- | --- | --- |
| GP0–GP7 | CAM_D0–CAM_D7 | OV5640 并行像素数据 |
| GP8 | CAM_VSYNC | 摄像头帧同步 |
| GP9 | CAM_HREF | 摄像头行有效 |
| GP10 | CAM_PCLK | 摄像头像素时钟 |
| GP11 | CAM_XCLK | 24 MHz 摄像头参考时钟 |
| GP15 | LCD_BL | 屏幕背光 |
| GP16 | LCD_DC | 屏幕数据/命令选择 |
| GP17 | LCD_CS | 屏幕片选 |
| GP18 | LCD_CLK | SPI0 时钟 |
| GP19 | LCD_DIN | SPI0 数据 |
| GP20 | LCD_RST | 屏幕复位 |
| GP21 | CAM_PWDN | 摄像头掉电控制，高电平有效 |
| GP22 | CAM_SIOD | OV5640 SCCB 数据 |
| GP23 | CAM_SIOC | OV5640 SCCB 时钟 |

引脚定义见 [`board_pins.h`](../boards/include/board_pins.h)。摄像头数据和时序信号 GP0–GP10 由 PIO 采集，图像以 RGB565 格式写入 SRAM；摄像头配置使用 I²C1。

## 板卡资料

- [RP2350-Touch-LCD-2-C 商品页](https://www.waveshare.net/shop/RP2350-Touch-LCD-2-C.htm)
- [RP2350-Touch-LCD-2 Wiki](https://www.waveshare.net/wiki/RP2350-Touch-LCD-2)
- [板卡原理图](https://files.waveshare.com/wiki/RP2350-Touch-LCD-2/RP2350-Touch-LCD-2.pdf)
