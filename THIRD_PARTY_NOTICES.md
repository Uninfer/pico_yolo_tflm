# 第三方声明

自有代码采用 Apache-2.0；以下导入代码保留原许可与版权声明。根许可证不覆盖部署模型或来源尚未确认的文件。

| 组件 | 路径 | 许可 / 来源 |
| --- | --- | --- |
| TensorFlow Lite Micro | `third_party/tflite_micro/` | Apache-2.0；TensorFlow Authors，许可证位于该目录 |
| CMSIS-DSP / CMSIS-NN | `third_party/CMSIS/` | Apache-2.0；Arm，许可证位于该目录 |
| FlatBuffers / gemmlowp / ruy | TFLM 的 `third_party/` | Apache-2.0；各目录附许可证 |
| KISS FFT | TFLM 的 `third_party/kissfft/` | BSD-3-Clause；保留 COPYING 与 LICENSES |
| OV5640 传感器与采集实现 | `src/drivers/camera/ov5640/ov5640.c`、`ov5640.h`、`camera_capture.c` | Waveshare；保留文件头的 MIT 风格授权声明 |
| ST7789 底层驱动 | `src/drivers/display/st7789/st7789.c`、`st7789.h` | Apache-2.0；Arm，保留原 SPDX 与版权声明 |
| Pico SDK 导入脚本 | `cmake/pico_sdk_import.cmake` | Raspberry Pi；BSD-3-Clause，文本位于 `LICENSES/` |

Pico SDK 为外部依赖，采用 [BSD-3-Clause](https://github.com/raspberrypi/pico-sdk/blob/2.1.1/LICENSE.TXT)。本项目对 TFLM 的裁剪仅移除单元测试、测试数据及辅助代码；生产算子实现保留。

## 需单独确认的来源

相机目录的 `camera_capture.h`、`camera_capture.pio`、`ov5640_camera.c` 和 `ov5640_camera.h` 未记录完整的原始来源和许可，不纳入顶层授权。

部署权重 `src/inference/models/fire/Fire_int8_weight.cc` 用于 fire/smoke 检测；其训练数据、作者及权重许可证尚无完整记录，不纳入 Apache-2.0 授权。更换或发布模型时记录来源链接、训练数据/权重许可、转换版本和文件哈希。未使用的示例帧数据已移除。

源码或固件发布包应附 `LICENSE`、`NOTICE`、本清单及所用组件的许可文本。外部硬件资料采用链接引用，其版权和商标权不由本项目授予。
