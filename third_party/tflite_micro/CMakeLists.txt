cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

# Set path variables.
set(TFLITE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tensorflow/lite)
set(SIGNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/signal)
set(MLIR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tensorflow/compiler/mlir/lite)

# Create the static library.
add_library(tflite_micro STATIC)

# Automatically collect source files.
file(GLOB_RECURSE TFLM_SOURCES CONFIGURE_DEPENDS
    ${TFLITE_DIR}/micro/*.cc
    ${TFLITE_DIR}/core/**/*.cc
    ${TFLITE_DIR}/kernels/*.cc
    ${TFLITE_DIR}/kernels/internal/**/*.cc
    ${MLIR_DIR}/core/api/*.cc
    ${MLIR_DIR}/schema/*.cc
    ${SIGNAL_DIR}/src/**/*.cc
)

# This library is built for firmware. Keep upstream unit tests and host-only
# testing helpers out of the embedded target.
list(FILTER TFLM_SOURCES EXCLUDE REGEX "_test[^/]*\\.cc$")
list(FILTER TFLM_SOURCES EXCLUDE REGEX "/testing/.*\\.cc$")
list(FILTER TFLM_SOURCES EXCLUDE REGEX "/(fake|mock)_[^/]*\\.cc$")
list(FILTER TFLM_SOURCES EXCLUDE REGEX "/test_helpers\\.cc$")

# Add source files.
target_sources(tflite_micro PRIVATE ${TFLM_SOURCES})

# Add include paths.
target_include_directories(tflite_micro PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TFLITE_DIR}
    ${TFLITE_DIR}/core/api
    ${TFLITE_DIR}/core/c
    ${TFLITE_DIR}/kernels
    ${TFLITE_DIR}/kernels/internal
    ${TFLITE_DIR}/kernels/internal/reference
    ${TFLITE_DIR}/micro
    ${TFLITE_DIR}/micro/arena_allocator
    ${TFLITE_DIR}/micro/kernels
    ${TFLITE_DIR}/micro/kernels/cmsis_nn # CMSIS-NN headers
    ${TFLITE_DIR}/micro/memory_planner
    ${TFLITE_DIR}/micro/tflite_bridge
    ${MLIR_DIR}/core/api
    ${MLIR_DIR}/schema
    ${SIGNAL_DIR}/micro/kernels
    ${SIGNAL_DIR}/src
    ${SIGNAL_DIR}/src/kiss_fft_wrappers
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/kissfft
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gemmlowp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gemmlowp/fixedpoint
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gemmlowp/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/flatbuffers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ruy

    ${CMAKE_CURRENT_SOURCE_DIR}/../CMSIS/cmsis_dsp/Include # CMSIS-DSP headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../CMSIS/cmsis_nn # CMSIS-NN headers
)

# Compile definitions
target_compile_definitions(tflite_micro PUBLIC
    CMSIS_NN  # Define the CMSIS_NN macro.
    TF_LITE_STATIC_MEMORY
    # TF_LITE_USE_GLOBAL_CMATH_FUNCTIONS
    TF_LITE_USE_GLOBAL_MIN
    TF_LITE_USE_GLOBAL_MAX
)

# Compiler optimization and warning settings.
target_compile_options(tflite_micro PRIVATE
    -O3
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-error=sign-compare
    -Wno-error=double-promotion
    -Wno-error=attributes
    -Wno-error=shadow
)

# Link the SDK and the vendored CMSIS implementations.
target_link_libraries(tflite_micro PRIVATE
    pico_stdlib
    cmsis_nn
    cmsis_dsp
)
