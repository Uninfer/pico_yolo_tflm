# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_yolo C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(CMSIS/cmsis_nn)
add_subdirectory(CMSIS/cmsis_dsp)
add_subdirectory(tflite_micro)

##########################pico_yolo###########################
# Add executable. Default name is the project name, version 0.1
add_executable(pico_yolo
        pico_yolo/models/Fire_int8_weight.cc
        pico_yolo/main_functions.cc
        pico_yolo/utils.cc
        main/main.c
        main/DEV_Config.c
        main/GUI_Paint.c
        st7789/LCD_2in.c
        st7789/lcd.c
        ov7670/ov5640.c
        ov7670/cam.c
        ov7670/camera.c
        # pico_yolo/src/Camera_OV7670.c
)

# 添加头文件目录
target_include_directories(pico_yolo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/pico_yolo
    ${CMAKE_CURRENT_LIST_DIR}/ov7670
    ${CMAKE_CURRENT_LIST_DIR}/st7789
    ${CMAKE_CURRENT_LIST_DIR}/main
)

# Generate PIO header for image.pio
pico_generate_pio_header(pico_yolo ${CMAKE_CURRENT_LIST_DIR}/ov7670/picampinos.pio)

# pull in common dependencies
target_link_libraries(pico_yolo
                      pico_stdlib
                      tflite_micro
                      cmsis_nn    # 添加 CMSIS-NN 库
                      cmsis_dsp   # 添加 CMSIS-DSP 库
                      hardware_spi # 添加硬件 SPI 库，驱动 ST7789

                      hardware_dma
                      hardware_i2c
                      hardware_pio
                      hardware_pwm
                      hardware_timer
                      pico_multicore    # 添加多核支持
)

# 使能 USB 输出，关闭 UART
pico_enable_stdio_usb(pico_yolo 1)
pico_enable_stdio_uart(pico_yolo 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(pico_yolo)
