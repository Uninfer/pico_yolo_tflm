# Architecture

This project is a compact, real-time fire and smoke detection pipeline built for the Waveshare RP2350-Touch-LCD-2-C board. Camera capture, display refresh, and inference run in separate stages. With the single frame buffer, the next capture waits for display and preprocessing reads to finish; model execution can overlap subsequent capture and display.

## 1. System overview

| Module | Responsibility |
| --- | --- |
| `src/app/main.c` | Boot flow, clock setup, dual-core scheduling, buffer ownership, and detection state |
| `src/drivers/camera/ov5640/` | SCCB configuration, PIO capture, DMA transfers, and frame-ready signaling |
| `src/drivers/display/st7789/` | LCD refresh, full-frame writes, and detection-box drawing |
| `src/inference/pico_yolo/` | Image conversion, TFLM invocation, postprocessing, and NMS |
| `src/inference/models/fire/` | Fixed deployed model weights and metadata |

```text
OV5640 camera -> PIO/DMA -> RGB565 frame -> LCD
                               |
                               v
                         Resize + letterbox
                               |
                    TensorFlow Lite Micro + CMSIS-NN
                               |
                     Box restore + NMS -> LCD overlay
```

### Execution model

- Core 0 handles capture and display.
- Core 1 prepares model input from the newest available frame.
- Preprocessing releases the camera frame before the inference call begins.
- Mutexes protect both shared frame state and detection results.
- The display uses the latest completed detection results, which may come from an older frame than the image currently on screen. This reflects the asynchronous pipeline.

### Memory and runtime footprint

TFLM uses a single 150 KiB frame buffer, the default allocator, native Pad/Conv operators, and ordinary `Invoke` calls. The measured tensor arena usage is 253092 / 253952 bytes.

## 2. Model interface and processing contract

- Classes: `0=fire`, `1=smoke`; up to 10 boxes are displayed.
- A 240×320 source frame is resized to 144×192 with 24 pixels of padding on each side.
- Padding color: RGB = `(114, 114, 114)`.
- Model input format: NHWC `1×192×192×3` RGB INT8.
- Quantization: `scale = 1/255`, `zero_point = -128`.
- The adapter converts TFLM INT8 outputs into `xywh` coordinates in model-pixel space.
- Postprocessing restores coordinates back to the original image space and applies a confidence threshold of `0.40` and an IoU threshold of `0.30` for per-class NMS.

When replacing the model, verify the following carefully:

- Input/output tensor shapes
- Quantization parameters
- Class ordering
- Operator support
- RAM and arena requirements

See [Third-party notices](../THIRD_PARTY_NOTICES.md) for source and licensing information.

## 3. Recorded performance

The measurements below were collected on 2026-10-10 using the RP2350 at 300 MHz system/peripheral clocks and a real LCD SPI clock of 75 MHz. Logs span approximately 65 seconds, excluding the first three startup windows.

| Metric | Measured result |
| --- | ---: |
| Inference throughput | 6.55 FPS |
| Display frame updates | 14.79 FPS |
| First display of new results | 6.58 FPS |
| Model execution | 138.76 ms |
| Capture request to display of new results | 220.60 ms |

### Log semantics

- `INFER` logs: preprocessing, model runtime, and postprocessing timing
- `PIPE` logs: frame and new-result throughput
- `VIDEO` logs: image state, gain, and exposure

### Reproduction guidance

To reproduce or compare measurements:

1. Flash the repository firmware.
2. Keep the camera position and lighting stable.
3. Record the commit, model, board, and runtime clocks.
4. Collect at least 60 seconds of serial logs.
5. Exclude the first three startup windows.
6. Compute throughput as total counts divided by total reporting time.
7. Weight average timing results by the sample count in each window.

Performance records should use only firmware and logs originating from this repository.

### Interpretation notes

- Display updates measure how often the software writes a complete frame.
- Pipeline latency starts at the capture request and excludes exposure start and panel scanning.
- Lighting, target count, and clock settings all affect results.
- The model does not have a reproducible standalone accuracy benchmark; real-world validation remains necessary in the target environment.
