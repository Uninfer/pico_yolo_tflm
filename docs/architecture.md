# Architecture

## Modules and data flow

| Module | Responsibility |
| --- | --- |
| `src/app/main.c` | Clocks, module initialization, dual-core scheduling, shared frames, and detection results |
| `src/drivers/camera/ov5640/` | SCCB configuration, PIO sampling, DMA transfers, and frame completion state |
| `src/drivers/display/st7789/` | Full-frame SPI transfers and detection box drawing |
| `src/inference/pico_yolo/` | Inference interface, image conversion, engine invocation, and NMS |
| `src/inference/models/fire/` | Fixed deployed model |

```text
OV5640 → PIO/DMA → RGB565 frame → LCD
                       ↓
                Resize and letterbox
                       ↓
       Standard TensorFlow Lite Micro + CMSIS-NN
                       ↓
             Restore box coordinates/NMS → LCD
```

Core 0 manages capture and display; Core 1 prepares model input from the latest available frame. Preprocessing releases the camera frame before model invocation. Mutexes protect frame state and detection results. The display uses the latest completed results, so detection boxes and the displayed image may come from different frames.

TFLM uses a single frame buffer (150 KiB), the standard allocator, native Pad/Conv operators, and ordinary Invoke calls. Actual tensor arena usage is 253092 / 253952 bytes.

## Model interface

- Classes: `0=fire`, `1=smoke`; up to 10 boxes are displayed.
- A 240×320 frame is resized to 144×192, with 24 pixels of padding on each side. The padding RGB value is 114.
- Model input: NHWC `1×192×192×3` RGB INT8, scale=1/255, zero_point=-128.
- The adapter converts TFLM INT8 output to xywh coordinates in model pixel units. Postprocessing restores original image coordinates, using a confidence threshold of 0.40 and a per-class NMS IoU threshold of 0.30.

When replacing the model, check input/output shapes, quantization parameters, class order, operator support, and RAM requirements. See [Third-party notices](../THIRD_PARTY_NOTICES.md) for sources and licensing.

## Recorded performance

These are existing measurements for this project from 2026-10-10: RP2350, 300 MHz system/peripheral clocks, and an actual LCD SPI clock of 75 MHz. Logs were collected for approximately 65 seconds, excluding the first three startup windows.

| Metric | Measured result |
| --- | ---: |
| Inference throughput | 6.55 FPS |
| Display frame updates | 14.79 FPS |
| First display of new results | 6.58 FPS |
| Model execution | 138.76 ms |
| Capture request to display of new results | 220.60 ms |

Serial `INFER` logs record preprocessing, model, and postprocessing times; `PIPE` logs record frame and new-result throughput; `VIDEO` logs record image state, gain, and exposure.

To repeat the measurements, flash this project's firmware, keep camera position and lighting stable, and record the commit, model, board, and runtime clocks. Collect at least 60 seconds of serial logs and exclude the first three startup windows. Calculate throughput as total counts divided by total reporting time, and weight average timings by the sample count in each window. Performance records should use only firmware and logs from this repository.

Display updates measure how often the software writes a complete frame. Pipeline latency starts at the capture request and excludes exposure start and panel scanning. Lighting, target count, and clocks affect the results. The model has no reproducible standalone accuracy evaluation; example detection results require validation in the intended environment.
