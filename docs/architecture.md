# 运行架构

## 主要目录

```text
boards/                  RP2350 板型配置和项目 GPIO 定义
src/app/                 固件入口和应用调度
src/drivers/camera/      OV5640 PIO/DMA 采集
src/drivers/display/     ST7789 屏幕驱动和绘图
src/inference/yolo/      TFLite Micro 预处理、推理和后处理
src/inference/models/    量化 YOLO 模型
third_party/             TFLite Micro、CMSIS-NN 和 CMSIS-DSP
```

## 单帧推理流程

1. 摄像头通过 PIO/DMA 把 240 × 320 RGB565 图像采集到 SRAM。
2. Core 0 显示当前帧，并在 Core 1 空闲时请求一次推理。
3. Core 1 将图像预处理到 TFLM 输入张量；预处理期间 LCD 与它并行读取原始帧。
4. LCD 传输和预处理都完成后，DMA 才能复用帧缓冲区。TFLM 随后在独立输入张量上推理。
5. 推理结束后，Core 1 发布最新检测框；Core 0 在显示循环中叠加这些检测框。

推理忙时跳过新帧的推理请求，不排队处理过期图像。当前使用单个摄像头帧缓冲区，为 TFLM Tensor Arena 留出 SRAM。TFLM 已启用 CMSIS-NN 加速。
