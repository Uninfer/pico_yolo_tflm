#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "main_functions.h"
#include "models/fire/Fire_int8_weight.h"
#include "pico/sync.h"
#include "utils.h"
#include "board_pins.h"

// Core 0 may reuse the single RGB565 frame only after Core 1 has copied it into
// the TFLM input tensor. The model then owns its input independently.
extern semaphore_t g_frame_buffer_consumed;

// 全局变量
namespace {
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;
// Reserve the arena in SRAM1 and keep it 16-byte aligned for TFLM/CMSIS-NN.
constexpr int kTensorArenaSize = 248 * 1024;
__attribute__((section(".sram1"))) alignas(16) static uint8_t tensor_arena[kTensorArenaSize];
} // namespace

// Resize RGB565 to the model's int8 RGB tensor. The board's 240x320 -> 192x192
// mapping has exact rational steps; avoid per-pixel division and function calls.
void __not_in_flash_func(pre_process)(const uint16_t *input, int in_w, int in_h,
                                      int8_t *output, int out_w, int out_h)
{
    if (in_w == BOARD_LCD_WIDTH && in_h == BOARD_LCD_HEIGHT && out_w == 192 && out_h == 192) {
        int src_y = 0;
        int y_remainder = 0;
        for (int y = 0; y < out_h; ++y) {
            const uint16_t *src_row = input + src_y * in_w;
            int8_t *dst = output + y * out_w * 3;

            for (int x = 0; x < out_w; ++x) {
                // floor(5*x/4), written as x + floor(x/4).
                const uint16_t pixel = src_row[x + (x >> 2)];
                const uint8_t r5 = (uint8_t)((pixel >> 11) & 0x1f);
                const uint8_t g6 = (uint8_t)((pixel >> 5) & 0x3f);
                const uint8_t b5 = (uint8_t)(pixel & 0x1f);

                // Expand RGB565 to RGB888 before applying q = u8 - 128,
                // matching the model's input scale of 1/255 and zero point -128.
                dst[0] = (int8_t)((int)((r5 << 3) | (r5 >> 2)) - 128);
                dst[1] = (int8_t)((int)((g6 << 2) | (g6 >> 4)) - 128);
                dst[2] = (int8_t)((int)((b5 << 3) | (b5 >> 2)) - 128);
                dst += 3;
            }

            // floor(5*y/3), advanced with a small remainder accumulator.
            y_remainder += 5;
            src_y += y_remainder / 3;
            y_remainder %= 3;
        }
        return;
    }

    // Keep a generic fallback for other image/model dimensions.
    for (int y = 0; y < out_h; ++y) {
        const int src_y = y * in_h / out_h;
        const uint16_t *src_row = input + src_y * in_w;
        int8_t *dst = output + y * out_w * 3;
        for (int x = 0; x < out_w; ++x) {
            const uint16_t pixel = src_row[x * in_w / out_w];
            const uint8_t r5 = (uint8_t)((pixel >> 11) & 0x1f);
            const uint8_t g6 = (uint8_t)((pixel >> 5) & 0x3f);
            const uint8_t b5 = (uint8_t)(pixel & 0x1f);
            *dst++ = (int8_t)((int)((r5 << 3) | (r5 >> 2)) - 128);
            *dst++ = (int8_t)((int)((g6 << 2) | (g6 >> 4)) - 128);
            *dst++ = (int8_t)((int)((b5 << 3) | (b5 >> 2)) - 128);
        }
    }
}

// 模型输出后处理：解析 bbox + NMS
void post_process(int8_t *raw_data, BBox *boxes, int *box_count)
{
    *box_count = 0;

    // 遍历所有候选框, 遍历 output->dims->data[2] 个检测框
    for (int i = 0; i < output->dims->data[2]; i++) {
        // 计算类别置信度
        float max_class_score = -1;
        int class_id = -1;
        for (int c = 0; c < output->dims->data[1] - 4; c++) {   // 类别数为 output->dims->data[1] - 4
            int class_channel = 4 + c;
            float class_score = (raw_data[class_channel * output->dims->data[2] + i] - output->params.zero_point) *
                                output->params.scale;
            if (class_score > max_class_score) {
                max_class_score = class_score;
                class_id = c;
            }
        }

        if (max_class_score < CONF_THRESHOLD) {
            continue;
        }

        // 解码 bbox 中心点和宽高
        float x = (raw_data[i] - output->params.zero_point) * output->params.scale;
        float y = (raw_data[i + output->dims->data[2]] - output->params.zero_point) * output->params.scale;
        float w_raw = (raw_data[i + 2 * output->dims->data[2]] - output->params.zero_point) * output->params.scale;
        float h_raw = (raw_data[i + 3 * output->dims->data[2]] - output->params.zero_point) * output->params.scale;

        // 转换为 (x1,y1,x2,y2) 格式
        float x1 = x - 0.5f * w_raw;
        float y1 = y - 0.5f * h_raw;
        float x2 = x + 0.5f * w_raw;
        float y2 = y + 0.5f * h_raw;

        // 存储到 BBox 数组
        boxes[*box_count] = (BBox){x1, y1, x2, y2, max_class_score, class_id};
        (*box_count)++;

        if (*box_count >= MAX_BOXES)
            break;
    }

    // NMS 去重
    NMS(boxes, box_count);

}

// 初始化 PICO YOLO 模型
bool pico_yolo_init()
{
    // 加载模型
    model = tflite::GetModel(Fire_int8_weight);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        MicroPrintf("Model provided is schema version %d not equal "
                    "to supported version %d.",
                    model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // 注册需要的算子
    static tflite::MicroMutableOpResolver<17> resolver;
    resolver.AddMaxPool2D();
    resolver.AddConv2D();
    resolver.AddReshape();
    resolver.AddSoftmax();
    resolver.AddTranspose();
    // resolver.AddFullyConnected();
    resolver.AddQuantize();
    resolver.AddPad();
    resolver.AddDepthwiseConv2D();
    resolver.AddConcatenation();
    resolver.AddResizeNearestNeighbor();
    resolver.AddAveragePool2D();
    resolver.AddAdd();
    resolver.AddLogistic();
    // resolver.AddRelu();
    resolver.AddGather();
    resolver.AddMul();
    resolver.AddSub();
    resolver.AddStridedSlice();

    // 构建解释器
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // 分配张量内存
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        MicroPrintf("AllocateTensors() failed");
        return false;
    }

    // 获取输入输出张量
    input = interpreter->input(0);
    output = interpreter->output(0);
    if (input == nullptr || output == nullptr || input->type != kTfLiteInt8 || output->type != kTfLiteInt8) {
        MicroPrintf("Model input/output tensor type is not int8.");
        return false;
    }
    if (input->dims->size != 4 || input->dims->data[1] != 192 ||
        input->dims->data[2] != 192 || input->dims->data[3] != 3 ||
        output->dims->size != 3 || output->dims->data[1] != 6 ||
        output->dims->data[2] != 144) {
        MicroPrintf("Model tensor shape does not match the configured preprocessing/postprocessing.");
        return false;
    }

    printf("TFLM arena: %u / %u bytes\n",
           static_cast<unsigned>(interpreter->arena_used_bytes()),
           static_cast<unsigned>(kTensorArenaSize));
    return true;
}

// 运行一次推理 (含耗时统计)
void pico_yolo_infer(const uint16_t *RGB565_input, BBox *boxes, int *box_count)
{
    uint64_t start_time_pre = time_us_64();
    pre_process(RGB565_input, BOARD_LCD_WIDTH, BOARD_LCD_HEIGHT, input->data.int8, 192, 192);
    const uint64_t pre_process_time_us = time_us_64() - start_time_pre;

    // Core 0 can now reuse the camera frame while TFLM runs on its own tensor.
    sem_release(&g_frame_buffer_consumed);

    uint64_t start_time_infer = time_us_64();
    TfLiteStatus invoke_status = interpreter->Invoke();
    const uint64_t infer_time_us = time_us_64() - start_time_infer;

    if (invoke_status != kTfLiteOk) {
        MicroPrintf("Invoke failed!");
        *box_count = 0;
        return;
    }

    uint64_t start_time_post = time_us_64();
    post_process(output->data.int8, boxes, box_count);
    const uint64_t post_process_time_us = time_us_64() - start_time_post;

    static uint64_t report_window_start_us = 0;
    static uint64_t pre_process_sum_us = 0;
    static uint64_t inference_sum_us = 0;
    static uint64_t post_process_sum_us = 0;
    static uint32_t report_inference_count = 0;
    const uint64_t now_us = time_us_64();
    if (report_window_start_us == 0) {
        report_window_start_us = now_us;
    }
    pre_process_sum_us += pre_process_time_us;
    inference_sum_us += infer_time_us;
    post_process_sum_us += post_process_time_us;
    ++report_inference_count;

    const uint64_t report_period_us = now_us - report_window_start_us;
    if (report_period_us >= 1000000) {
        const uint64_t fps_x10 = (static_cast<uint64_t>(report_inference_count) * 10000000) /
                                 report_period_us;
        printf("YOLO %llu.%llu fps | preprocess %llu us | invoke %llu us | post %llu us\n",
               static_cast<unsigned long long>(fps_x10 / 10),
               static_cast<unsigned long long>(fps_x10 % 10),
               static_cast<unsigned long long>(pre_process_sum_us / report_inference_count),
               static_cast<unsigned long long>(inference_sum_us / report_inference_count),
               static_cast<unsigned long long>(post_process_sum_us / report_inference_count));
        report_window_start_us = now_us;
        pre_process_sum_us = 0;
        inference_sum_us = 0;
        post_process_sum_us = 0;
        report_inference_count = 0;
    }
}
