# pico_yolo_rp2350

在 RP2350 上运行火焰/烟雾 YOLO 检测，使用 **标准 TensorFlow Lite Micro + CMSIS-NN**、OV5640 摄像头和 ST7789 屏幕。输入画面为 240×320 RGB565，模型输入为 192×192 RGB INT8。

固定适配 Waveshare RP2350-Touch-LCD-2-C 摄像头套装。 双核运行：Core 0 采集与显示，Core 1 推理。当前采用单帧 RGB565 缓冲，标准 TFLM 张量工作区预留 248 KiB。

## 编译与烧录

需要 Pico SDK **2.1.1**（含子模块）、Arm GNU Toolchain、CMake **3.20+** 和 Ninja。设置 SDK 路径并确保 `arm-none-eabi-gcc` 在 PATH 中：

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

也可运行 `./scripts/build.sh`。生成文件为 `build/pico_yolo_rp2350.uf2`。

按住 BOOT/BOOTSEL 键连接 USB，把 UF2 复制到 RP2350 启动盘。Windows 可运行 `./scripts/flash.ps1`。启动后打开 USB 串口查看日志，串口工具可设置 115200。

默认系统/外设时钟为 **300 MHz**，属于超频；改用 150 MHz 时，在配置命令中加入 `-DPICO_YOLO_SYS_CLOCK_KHZ=150000`。以下性能只对应 300 MHz 配置。

## 目录与说明

```text
src/
  app/                       应用入口与双核调度
  drivers/camera/ov5640/      摄像头、PIO 与 DMA
  drivers/display/st7789/    屏幕驱动
  inference/pico_yolo/       引擎适配、预处理与后处理
  inference/models/fire/     部署模型
boards/                      板级引脚与 Pico SDK 板型
third_party/                 引擎/Arm 依赖及其许可证
cmake/                       Pico SDK 导入
scripts/                     编译与烧录
```

- [架构说明](docs/architecture.md)：模块职责、帧流程、模型接口与当前性能。
- [硬件说明](docs/hardware.md)：引脚、相机参数和常见故障。
- [贡献指南](CONTRIBUTING.md)：提交 Issue/PR 和验证方式。
- [第三方声明](THIRD_PARTY_NOTICES.md)：依赖来源与模型授权边界。

## 开源许可

项目自有代码与文档采用 [Apache-2.0](LICENSE)，适用范围见 [NOTICE](NOTICE)。第三方文件保留原许可；模型权重及示例图像的授权状态单独记录在 [第三方声明](THIRD_PARTY_NOTICES.md)。
