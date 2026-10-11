# pico_yolo_tflm

Fire and smoke detection on RP2350 using YOLO, **standard TensorFlow Lite Micro + CMSIS-NN**, an OV5640 camera, and an ST7789 display. Camera frames are 240×320 RGB565; model input is 192×192 RGB INT8.

The firmware targets the Waveshare RP2350-Touch-LCD-2-C camera kit. Core 0 handles capture and display; Core 1 runs inference. It uses a single RGB565 frame buffer and reserves 248 KiB for the standard TFLM tensor arena.

## Build and flash

Requirements: Pico SDK **2.1.1** (including submodules), Arm GNU Toolchain, CMake **3.20+**, and Ninja. Set the SDK path and ensure `arm-none-eabi-gcc` is on PATH:

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Alternatively, run `./scripts/build.sh`. The output is `build/pico_yolo_tflm.uf2`.

Hold BOOT/BOOTSEL while connecting USB, then copy the UF2 file to the RP2350 boot drive. On Windows, you can run `./scripts/flash.ps1`. After startup, open the USB serial port to view logs; configure your serial terminal for 115200 baud.

The default system and peripheral clocks are **300 MHz**, which is an overclock. To use 150 MHz, add `-DPICO_YOLO_SYS_CLOCK_KHZ=150000` to the configuration command. The documented performance figures apply only to the 300 MHz configuration.

## Project layout and documentation

```text
src/
  app/                       Application entry point and dual-core scheduling
  drivers/camera/ov5640/      Camera, PIO, and DMA
  drivers/display/st7789/    Display driver
  inference/pico_yolo/       Engine integration, preprocessing, and postprocessing
  inference/models/fire/     Deployed model
boards/                      Board pins and Pico SDK board definitions
third_party/                 Inference and Arm dependencies with their licenses
cmake/                       Pico SDK import
scripts/                     Build and flash helpers
```

- [Architecture](docs/architecture.md): module responsibilities, frame flow, model interface, and current performance.
- [Hardware](docs/hardware.md): pins, camera settings, and troubleshooting.
- [Contributing](CONTRIBUTING.md): issues, pull requests, and validation.
- [Third-party notices](THIRD_PARTY_NOTICES.md): dependency sources and model licensing boundaries.

## License

Project-authored code and documentation are licensed under [Apache-2.0](LICENSE); see [NOTICE](NOTICE) for scope. Third-party files retain their original licenses. The licensing status of model weights and sample images is recorded separately in [Third-party notices](THIRD_PARTY_NOTICES.md).
