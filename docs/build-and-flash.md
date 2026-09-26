# 构建与烧录

## 构建

需要 Pico SDK 2.1.1、CMake、Ninja 和 Arm GNU Toolchain。设置 `PICO_SDK_PATH` 后，在仓库根目录运行：

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

项目已内置 Waveshare RP2350-Touch-LCD-2-C 板型配置。生成的固件为 `build/pico_yolo_rp2350.uf2`。

默认系统和外设时钟为 300 MHz。要使用 150 MHz 基准时钟，配置时添加 `-DPICO_YOLO_SYS_CLOCK_KHZ=150000`。300 MHz 设置和性能说明见[性能文档](performance.md)。

## 烧录

1. 按住板上的 BOOT 键并连接 USB，出现 RP2350 启动盘后松开 BOOT。
2. 将 `build/pico_yolo_rp2350.uf2` 复制到启动盘。
3. 启动盘断开后，板子会重新启动运行固件。

## 运行信息

固件启用 USB CDC。重启后打开对应串口，波特率设为 115200，可查看时钟、模型初始化和推理耗时信息。

发布固件时，请附上 `LICENSE`、`NOTICE`、`THIRD_PARTY_NOTICES.md` 和清单中适用的许可证文本。当前模型权重及示例帧的再分发许可仍待核实，详见[模型说明](model.md)。
