# 硬件说明

RP2350、OV5640 DVP 摄像头和 240×320 ST7789 SPI 屏幕。TFLM 固定使用 Waveshare RP2350-Touch-LCD-2-C 板型；Uninfer 默认使用 Pico 2 W 板型，两工程的外设引脚相同。

| GPIO | 信号 |
| --- | --- |
| GP0–GP7 | 摄像头 D0–D7 |
| GP8 / GP9 / GP10 | VSYNC / HREF / PCLK |
| GP11 | 摄像头 XCLK（PWM，请求 24 MHz，实际随系统时钟量化） |
| GP21 | 摄像头 PWDN（高电平掉电） |
| GP22 / GP23 | SCCB SDA / SCL（I²C1） |
| GP15 / GP16 / GP17 | LCD BL / DC / CS |
| GP18 / GP19 / GP20 | LCD SCK / MOSI / RST（SPI0） |

接线时核对模组电源电压并共地。板卡资料见 [Waveshare Wiki](https://www.waveshare.net/wiki/RP2350-Touch-LCD-2)。

## 当前相机与显示参数

摄像头输出 240×320 RGB565。300 MHz 配置下基础传感器输出约 29.4 FPS，LCD SPI 实际约 75 MHz；整帧传输约 18 ms。当前保留上一轮已测试的相机参数：6 倍自动增益上限、手动降噪阈值 16、降低锐化、夜间自动曝光和 50 Hz 防频闪；后续极暗环境实验已撤回。

启动日志会读回相机参数。该参数组主要在偏暗室内 LED 环境测试；极暗时仍可能出现噪点、明暗跳动或横条纹，当前没有验证彻底消除。不能仅凭基础帧率换算实际曝光时间。

## 常见问题

| 现象 | 检查项 |
| --- | --- |
| 找不到 Pico SDK | 设置 `PICO_SDK_PATH`，初始化 SDK 子模块 |
| 找不到交叉编译器 | 将 Arm GNU Toolchain 的 `bin` 加入 PATH |
| 未出现 RP2350 启动盘 | 按住 BOOT/BOOTSEL 重新连接 USB，检查数据线 |
| 黑屏或相机无图 | 检查引脚、PWDN、电源和 SCCB 初始化日志 |
| 彩点、横条纹或暗光闪烁 | 检查接线长度、供电与灯具 PWM，尝试稳定补光 |
| 随机重启 | 检查供电，切回 150 MHz 基准时钟验证 |

修改驱动参数前先记录原参数和同场景画面，再比较画质、吞吐和延迟。
