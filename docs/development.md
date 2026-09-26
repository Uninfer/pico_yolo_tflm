# 开发说明

## 代码位置

- `src/app/`：启动流程和相机、显示、推理调度。
- `src/drivers/camera/ov5640/`：OV5640 配置以及 PIO/DMA 图像采集。
- `src/drivers/display/st7789/`：屏幕通信和绘图。
- `src/inference/yolo/`：图像预处理、TFLite Micro 调用和检测框后处理。
- `src/inference/models/`：量化模型数据。
- `boards/include/board_pins.h`：本板使用的 GPIO 定义。
- `third_party/`：TFLite Micro、CMSIS-NN 和 CMSIS-DSP 上游依赖。

## 图像与模型

摄像头帧为 240 × 320 RGB565，预处理缩放为 192 × 192 RGB 并转换成 int8 输入。当前模型类别为 `fire` 和 `smoke`。修改 PIO 采集时请编辑 `.pio` 源文件，生成的头文件由构建系统更新。

模型权重和示例帧的来源、再分发许可尚未确认。更新或分发模型前，核实并记录模型来源、许可证、输入输出量化参数和类别顺序，详见[模型说明](model.md)及 [`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md)。

构建命令见[构建与烧录](build-and-flash.md)。
