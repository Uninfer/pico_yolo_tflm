# 性能与帧流水线

固件使用 TFLite Micro（TFLM）和 CMSIS-NN 执行当前量化模型。

## 帧所有权和推理调度

当前固件保留一个 240 × 320 RGB565 帧缓冲区。RP2350 的 SRAM 预算还要容纳约 248 KiB 的 TFLM Tensor Arena；再添加约 150 KiB 的 RGB565 帧会挤压 arena 空间。模型常量现已放在 Flash，释放约 90 KiB 的可写 SRAM。

单帧方案维持以下流程：

1. 摄像头 DMA 写入帧缓冲区，完成后 Core 0 收到帧就绪状态。
2. 如果 Core 1 空闲，Core 0 发出一次推理请求；Core 1 预处理当前图像，同时 Core 0 将其发送到 LCD。
3. 预处理复制完 TFLM 输入张量且 LCD 发送结束后，Core 0 才复用原始帧缓冲区。
4. Core 1 在独立的 TFLM 输入张量上执行推理，摄像头可在此期间继续采集和显示。
5. 如果 Core 1 仍在推理，Core 0 跳过新帧的推理请求，不排队旧帧；推理完成后再从最新帧继续。

`g_inference_request` 和 `g_frame_buffer_consumed` 分别表示新推理任务和原始帧已可复用。检测结果通过互斥锁发布，Core 0 在 LCD 刷新后取一份快照并绘制，避免 Core 1 推理耗时阻塞实时预览。

## 已做的优化

- 模型权重声明为 `const`，直接从 Flash 读取，避免复制到 SRAM 的可写 `.data` 区域。
- 为板上 240 × 320 到模型 192 × 192 的最近邻缩放增加固定尺寸路径，避免逐像素整除和额外 RGB565 转换函数调用；通道使用位复制扩展到完整 RGB888，再按模型输入 `scale=1/255`、`zero_point=-128` 写入 int8。
- 将预处理函数放入 SRAM 执行。
- 移除逐候选框和逐帧详细串口输出；现在每秒汇总一次推理 FPS、预处理、Invoke 和后处理平均耗时。
- 检测框从逐像素重设 LCD 窗口改为每条边设置一次窗口，再批量发送 RGB565 像素。
- 推理只由新帧请求触发；推理完成前不启动下一次 TFLM 调用。
- `clk_peri` 跟随 `clk_sys`，避免外设总线维持低频时钟。

## 时钟配置

默认系统时钟为 300 MHz，以提高 TFLM Invoke 速度。RP2350 数据手册列出的处理器基准频率为 150 MHz，因此 300 MHz 属板级超频；固件会先将 VREG 调到 SDK 最大常规档位 1.30 V。需要使用基准频率时，可通过 CMake 将时钟设为 150 MHz。[RP2350 数据手册](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)；[Pico SDK 2.1.1 VREG API](https://github.com/raspberrypi/pico-sdk/blob/2.1.1/src/rp2_common/hardware_vreg/include/hardware/vreg.h)

```sh
cmake -S . -B build -G Ninja \
  -DPICO_YOLO_SYS_CLOCK_KHZ=300000
cmake --build build
```

高于 150 MHz 时，固件先设置 SDK 支持的最高 VREG 档位，再请求目标时钟；若 SDK 无法配置目标时钟则回退到 150 MHz。可用 `-DPICO_YOLO_SYS_CLOCK_KHZ=150000` 构建基准时钟版本。修改已有构建目录中的时钟时，CMake 会重新配置；需要完全清理时请删除 `build/` 后重新运行命令。

## 板上测量

启动后 USB CDC 会报告实际系统/外设时钟、TFLM arena 占用和每秒推理统计。LCD 传输耗时、相机丢帧和端到端帧率仍需上板单独测量；300 MHz 预计能显著缩短 Invoke 时间，但需要在这块板上测量确认。

还需用固定图像核对 RGB565 位序、颜色通道、缩放结果和检测框坐标。若后续模型改用 letterbox 或不同量化参数，应一并更新预处理与坐标还原逻辑。
