cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default performance clock; use 150000 kHz for the RP2350 baseline.
set(PICO_YOLO_SYS_CLOCK_KHZ "300000" CACHE STRING "RP2350 system/peripheral clock in kHz")
if(NOT PICO_YOLO_SYS_CLOCK_KHZ MATCHES "^[0-9]+$")
    message(FATAL_ERROR "PICO_YOLO_SYS_CLOCK_KHZ must be an integer in kHz")
endif()

set(PICO_BOARD waveshare_rp2350_touch_lcd_2 CACHE STRING "Fixed target board" FORCE)
set(PICO_BOARD_HEADER_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/boards/include/boards"
    CACHE STRING "Project board definitions" FORCE)

include(pico_sdk_import.cmake)

project(pico_yolo_rp2350 C CXX ASM)
pico_sdk_init()

add_subdirectory(third_party/CMSIS/cmsis_nn)
add_subdirectory(third_party/CMSIS/cmsis_dsp)
add_subdirectory(third_party/tflite_micro)

set(PROJECT_BOARD_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/boards/include")
set(PROJECT_CAMERA_DIR "${CMAKE_CURRENT_LIST_DIR}/src/drivers/camera/ov5640")
set(PROJECT_DISPLAY_DIR "${CMAKE_CURRENT_LIST_DIR}/src/drivers/display/st7789")
set(PROJECT_INFERENCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src/inference")

add_library(camera_ov5640 STATIC
    src/drivers/camera/ov5640/ov5640.c
    src/drivers/camera/ov5640/camera_capture.c
    src/drivers/camera/ov5640/ov5640_camera.c
)
target_include_directories(camera_ov5640 PUBLIC
    "${PROJECT_BOARD_INCLUDE_DIR}"
    "${PROJECT_CAMERA_DIR}"
)
target_link_libraries(camera_ov5640 PUBLIC
    pico_stdlib
    hardware_dma
    hardware_i2c
    hardware_pio
    hardware_pwm
)
pico_generate_pio_header(camera_ov5640
    "${PROJECT_CAMERA_DIR}/camera_capture.pio")

add_library(display_st7789 STATIC
    src/drivers/display/st7789/st7789.c
    src/drivers/display/st7789/lcd.c
)
target_include_directories(display_st7789 PUBLIC
    "${PROJECT_BOARD_INCLUDE_DIR}"
    "${PROJECT_DISPLAY_DIR}"
)
target_link_libraries(display_st7789 PUBLIC
    pico_stdlib
    hardware_dma
    hardware_spi
)

add_library(yolo_inference STATIC
    src/inference/yolo/main_functions.cc
    src/inference/yolo/utils.cc
    src/inference/models/fire/Fire_int8_weight.cc
)
target_include_directories(yolo_inference PUBLIC
    "${PROJECT_INFERENCE_DIR}"
    "${PROJECT_INFERENCE_DIR}/yolo"
    "${PROJECT_BOARD_INCLUDE_DIR}"
)
target_link_libraries(yolo_inference PUBLIC
    pico_stdlib
    pico_multicore
    tflite_micro
    cmsis_nn
    cmsis_dsp
)

add_executable(pico_yolo_rp2350
    src/app/main.c
)
target_include_directories(pico_yolo_rp2350 PRIVATE
    "${PROJECT_BOARD_INCLUDE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/src/app"
)
target_link_libraries(pico_yolo_rp2350 PRIVATE
    pico_stdlib
    pico_multicore
    camera_ov5640
    display_st7789
    yolo_inference
    hardware_clocks
    hardware_i2c
    hardware_spi
    hardware_timer
    hardware_vreg
)
target_compile_definitions(pico_yolo_rp2350 PRIVATE
    PICO_YOLO_SYS_CLOCK_KHZ=${PICO_YOLO_SYS_CLOCK_KHZ}
)

pico_enable_stdio_usb(pico_yolo_rp2350 1)
pico_enable_stdio_uart(pico_yolo_rp2350 0)
pico_add_extra_outputs(pico_yolo_rp2350)
