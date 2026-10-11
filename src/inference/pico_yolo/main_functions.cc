#include "pico_yolo_inference.h"
#include "pico_yolo_processing.h"
#include "models/fire/Fire_int8_weight.h"
#include "pico/stdlib.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include <stdio.h>

namespace
{
const size_t kTensorArenaSize = 248 * 1024;
__attribute__((section(".sram1"))) alignas(16) static uint8_t tensor_arena[kTensorArenaSize];
const tflite::Model *model;
tflite::MicroInterpreter *interpreter;
TfLiteTensor *input;
TfLiteTensor *output;
} // namespace

int pico_yolo_init(void)
{
    model = tflite::GetModel(Fire_int8_weight);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        printf("Unsupported TFLM model schema\n");
        return -1;
    }
    // Register the required operators.
    static tflite::MicroMutableOpResolver<17> resolver;
    resolver.AddMaxPool2D();
    resolver.AddConv2D();
    resolver.AddReshape();
    resolver.AddSoftmax();
    resolver.AddTranspose();
    resolver.AddQuantize();
    resolver.AddPad();
    resolver.AddDepthwiseConv2D();
    resolver.AddConcatenation();
    resolver.AddResizeNearestNeighbor();
    resolver.AddAveragePool2D();
    resolver.AddAdd();
    resolver.AddLogistic();
    resolver.AddGather();
    resolver.AddMul();
    resolver.AddSub();
    resolver.AddStridedSlice();

    // As in the original version, the standard allocator manages the full input, Pad, and convolution tensors.
    static tflite::MicroInterpreter instance(model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &instance;
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        printf("TFLM AllocateTensors failed\n");
        return -3;
    }
    input = interpreter->input(0);
    output = interpreter->output(0);
    if (input == nullptr || output == nullptr || input->type != kTfLiteInt8 ||
        output->type != kTfLiteInt8 || input->dims->size != 4 || input->dims->data[0] != 1 ||
        input->dims->data[1] != MODEL_HEIGHT || input->dims->data[2] != MODEL_WIDTH ||
        input->dims->data[3] != 3 || input->params.zero_point != -128 ||
        input->params.scale != 1.0f / 255.0f || output->dims->size != 3 ||
        output->dims->data[0] != 1 || output->dims->data[1] != OUTPUT_CHANNELS ||
        output->dims->data[2] != OUTPUT_ANCHORS) {
        return -4;
    }
    printf("TFLM arena: %u / %u bytes (standard allocator)\n",
           static_cast<unsigned>(interpreter->arena_used_bytes()),
           static_cast<unsigned>(sizeof(tensor_arena)));
    printf("YOLO image: camera=240x320 resized=144x192 input=192x192 pad=left/right 24\n");
    return 0;
}

int pico_yolo_preprocess_frame(const uint16_t *rgb565, uint32_t *elapsed_us)
{
    if (interpreter == nullptr || rgb565 == nullptr) {
        return -1;
    }
    const uint64_t started = time_us_64();
    // Preprocessing writes to TFLM's own input tensor without overwriting the camera frame.
    preprocess_rgb565(rgb565, input->data.int8);
    if (elapsed_us != nullptr) {
        *elapsed_us = static_cast<uint32_t>(time_us_64() - started);
    }
    // Input copying is complete; capture can reuse the camera buffer after the display finishes reading it.
    pico_yolo_release_frame();
    return 0;
}

int pico_yolo_run_preprocessed(pico_yolo_box_t *boxes, uint32_t *box_count, uint32_t *inference_us,
                               uint32_t *postprocess_us)
{
    if (interpreter == nullptr || boxes == nullptr || box_count == nullptr) {
        return -1;
    }
    const uint64_t started = time_us_64();
    const TfLiteStatus status = interpreter->Invoke();
    const uint64_t invoked = time_us_64();
    if (inference_us != nullptr) {
        *inference_us = static_cast<uint32_t>(invoked - started);
    }
    if (status != kTfLiteOk) {
        *box_count = 0;
        pico_yolo_release_frame();
        return -2;
    }
    // TFLite exports normalized INT8 xywh; Uninfer exports pixel float32 xywh.
    // Normalize this engine-specific output representation before shared NMS.
    static float values[OUTPUT_CHANNELS * OUTPUT_ANCHORS];
    for (unsigned i = 0; i < OUTPUT_CHANNELS * OUTPUT_ANCHORS; ++i) {
        const float value =
            (output->data.int8[i] - output->params.zero_point) * output->params.scale;
        if (i < 4U * OUTPUT_ANCHORS) {
            values[i] = value * MODEL_WIDTH;
        } else {
            values[i] = value;
        }
    }
    *box_count = pico_yolo_postprocess_float(values, boxes, PICO_YOLO_MAX_BOXES);
    if (postprocess_us != nullptr) {
        *postprocess_us = static_cast<uint32_t>(time_us_64() - invoked);
    }
    return 0;
}

int pico_yolo_infer(const uint16_t *rgb565, pico_yolo_box_t *boxes, uint32_t *box_count)
{
    if (pico_yolo_preprocess_frame(rgb565, nullptr) != 0) {
        return -1;
    }
    return pico_yolo_run_preprocessed(boxes, box_count, nullptr, nullptr);
}
