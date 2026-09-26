# 第三方许可与来源清单

本清单记录仓库中已识别的第三方代码和数据。修改或再发布上游文件时，请保留其原有版权、许可和 SPDX 声明。

## 已识别的第三方代码

| 组件 | 仓库路径 | 许可与归属 | 许可文本 |
| --- | --- | --- | --- |
| TensorFlow Lite for Microcontrollers（TFLM） | `third_party/tflite_micro/` | Apache-2.0；TensorFlow Authors | `third_party/tflite_micro/LICENSE` |
| CMSIS-DSP、CMSIS-NN | `third_party/CMSIS/` | Apache-2.0；Arm Limited 及贡献者 | `third_party/CMSIS/LICENSE` |
| FlatBuffers（TFLM 内置） | `third_party/tflite_micro/third_party/flatbuffers/` | Apache-2.0；Google Inc. 及贡献者 | `third_party/tflite_micro/third_party/flatbuffers/LICENSE` |
| gemmlowp（TFLM 内置） | `third_party/tflite_micro/third_party/gemmlowp/` | Apache-2.0；Gemmlowp Authors | `third_party/tflite_micro/third_party/gemmlowp/LICENSE` |
| ruy（TFLM 内置） | `third_party/tflite_micro/third_party/ruy/` | Apache-2.0；Google 及贡献者 | `third_party/tflite_micro/third_party/ruy/LICENSE` |
| KISS FFT（TFLM 内置） | `third_party/tflite_micro/third_party/kissfft/` | BSD-3-Clause；Mark Borgerding | 该目录下的 `COPYING` 和 `LICENSES/BSD-3-Clause` |
| Waveshare OV5640 初始化与采集代码 | `src/drivers/camera/ov5640/ov5640.c`、`camera_capture.c`、`ov5640.h` | Waveshare team；各文件保留了 MIT 风格授权声明 | 各源文件头部的声明 |
| ST7789 驱动 | `src/drivers/display/st7789/st7789.c`、`st7789.h` | Copyright (c) 2021 Arm Limited and Contributors；Apache-2.0 | 根目录 `LICENSE`，源文件 SPDX 声明 |
| Pico SDK 导入脚本 | `pico_sdk_import.cmake` | Copyright 2020 Raspberry Pi (Trading) Ltd.；BSD-3-Clause，声明保留在脚本内 | `LICENSES/BSD-3-Clause-Raspberry-Pi.txt` |

Pico SDK 本身是外部构建依赖，没有随本仓库一并 vendoring。其许可证为 BSD-3-Clause，参见 [Pico SDK 许可证](https://github.com/raspberrypi/pico-sdk/blob/master/LICENSE.TXT)。

## 来源或授权仍待确认的代码

以下文件没有单独的许可或来源声明。在确认来源前，它们不属于本项目 Apache-2.0 授权范围：

- `src/drivers/camera/ov5640/camera_capture.h`
- `src/drivers/camera/ov5640/camera_capture.pio`
- `src/drivers/camera/ov5640/ov5640_camera.c` 和 `ov5640_camera.h`

## 来源或再分发许可仍待确认的数据

以下数据缺少来源、作者、数据集和再分发许可记录。在权利得到确认前，它们**不属于本项目 Apache-2.0 授权范围**：

- `src/inference/models/fire/Fire_int8_weight.cc`：嵌入式 TFLite 模型，头文件将类别标为 `fire` 和 `smoke`；尚无模型作者、训练数据、来源仓库或模型许可记录。
- `src/inference/assets/video_frames.h`：嵌入式 RGB565 图像/帧数据；尚无来源及再分发条款。

发布源码包或包含这些素材的固件前，应先确认其来源并记录适用许可，或更换为再分发权利明确的素材。模型还应附模型卡，记录训练数据和评估信息，详见[模型说明](docs/model.md)。

分发含第三方代码的固件时，请随二进制发布包一起提供本清单以及适用的许可证文本。

## 其他权利

本项目许可证不授权使用 Waveshare 或 Arm 商标，也不授权再分发供应商网站上的原理图、产品照片或其他资料。硬件资料以供应商链接形式引用；再发布这些资料前，请先查看各自的使用条款。
