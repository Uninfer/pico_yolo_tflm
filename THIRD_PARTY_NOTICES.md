# Third-party notices

Project-authored code is licensed under Apache-2.0. Imported code listed below retains its original license and copyright notices. The root license does not cover the deployed model or files whose provenance has not been confirmed.

| Component | Path | License / source |
| --- | --- | --- |
| TensorFlow Lite Micro | `third_party/tflite_micro/` | Apache-2.0; TensorFlow Authors; license included in that directory |
| CMSIS-DSP / CMSIS-NN | `third_party/CMSIS/` | Apache-2.0; Arm; licenses included in their directories |
| FlatBuffers / gemmlowp / ruy | TFLM's `third_party/` | Apache-2.0; licenses included in the respective directories |
| KISS FFT | TFLM's `third_party/kissfft/` | BSD-3-Clause; COPYING and LICENSES retained |
| OV5640 sensor and capture implementation | `src/drivers/camera/ov5640/ov5640.c`, `ov5640.h`, `camera_capture.c` | Waveshare; MIT-style permission notices retained in file headers |
| ST7789 low-level driver | `src/drivers/display/st7789/st7789.c`, `st7789.h` | Apache-2.0; Arm; original SPDX and copyright notices retained |
| Pico SDK import script | `cmake/pico_sdk_import.cmake` | Raspberry Pi; BSD-3-Clause; license text in `LICENSES/` |

Pico SDK is an external dependency licensed under [BSD-3-Clause](https://github.com/raspberrypi/pico-sdk/blob/2.1.1/LICENSE.TXT). TFLM trimming in this project removes only unit tests, test data, and helper code; production operator implementations are retained. English translations of comments in the vendored CMake build files do not change build behavior.

## Sources requiring separate verification

The camera files `camera_capture.h`, `camera_capture.pio`, `ov5640_camera.c`, and `ov5640_camera.h` do not have complete records of their original sources and licenses. They are excluded from the top-level license grant.

The deployed weights in `src/inference/models/fire/Fire_int8_weight.cc` are used for fire/smoke detection. Complete records of the training data, author, and weight license are not available; the weights are excluded from the Apache-2.0 license grant. When replacing or distributing a model, record its source link, training-data and weight licenses, conversion version, and file hash. Unused sample frame data has been removed.

Source or firmware distributions should include `LICENSE`, `NOTICE`, this document, and the license texts of the components used. External hardware documentation is referenced through links; this project does not grant rights to its copyrights or trademarks.
