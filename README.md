# pico_yolo_rp2350

在 Waveshare [RP2350-Touch-LCD-2-C](https://www.waveshare.net/shop/RP2350-Touch-LCD-2-C.htm) 上运行的 YOLO 固件：OV5640 摄像头采集、ST7789 屏幕预览，以及基于 TFLite Micro 的目标检测。

本项目固定适配该板卡和附带的 OV5640 摄像头，不需要在构建命令中选择板型。固件默认使用 300 MHz 系统时钟提升推理速度；这是高于 150 MHz 基准频率的超频设置，可通过 CMake 参数切回基准频率。

## 构建与烧录

安装 Pico SDK 2.1.1、CMake、Ninja 和 Arm GNU Toolchain，并设置 `PICO_SDK_PATH`：

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build -G Ninja
cmake --build build
```

生成文件为 `build/pico_yolo_rp2350.uf2`。按住板上的 BOOT 键连接 USB，出现 RP2350 启动盘后，将 UF2 复制进去。完整步骤见[构建与烧录](docs/build-and-flash.md)。

## 项目说明

- [硬件与引脚](docs/hardware.md)：本项目使用的摄像头、屏幕和 GPIO。
- [运行架构](docs/architecture.md)：相机帧、双核推理与显示流程。
- [性能说明](docs/performance.md)：帧调度、推理统计和时钟设置。
- [模型说明](docs/model.md)：输入输出、类别及模型许可状态。
- [故障排查](docs/troubleshooting.md)：构建、烧录、摄像头、屏幕和推理问题。
- [贡献指南](CONTRIBUTING.md)

项目自有代码和文档采用 Apache-2.0。当前模型权重和示例帧的来源及再分发许可尚未确认，不属于项目 Apache-2.0 授权范围；对外发布固件前请先核实，详情见 [模型说明](docs/model.md) 和[第三方许可清单](THIRD_PARTY_NOTICES.md)。
