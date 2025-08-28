# 接线

## 摄像头

| Raspberry Pi Pico | OV7670            |
| ----------------- | ----------------- |
| GND               | GND               |
| 3V3(OUT)          | VCC (3.3)         |
| GP3               | XCLK              |
| GP4  (I2C0 SDA)   | SDA[^i2c_pullups] |
| GP6               | D0                |
| GP7               | D1                |
| GP8               | D2                |
| GP9               | D3                |
| GP10              | D4                |
| GP11              | D5                |
| GP12              | D6                |
| GP13              | D7                |
| GP14              | PCLK              |
| GP15              | HREF              |
| GP16              | VSYNC             |
| GP17              | RST               |
| GP21 (I2C0 SCL)   | SCL[^i2c_pullups] |

## 屏幕

| Raspberry Pi Pico | st7789            |
| ----------------- | ----------------- |
| GND               | GND               |
| 3V3(OUT)          | VCC (3.3)         |
| GP22              | BL                |
| GP26              | CS                |
| GP20              | DC                |
| GP27              | RST               |
| GP19              | SDA               |
| GP18              | SCL               |