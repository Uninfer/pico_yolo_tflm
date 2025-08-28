#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "config.h"
#include "main_functions.h"
#include "models/Fire_int8_weight.h"
#include "pico/sync.h"
#include "utils.h"

extern mutex_t g_image_buffer_lock;
extern mutex_t g_box_lock;

// 全局变量
namespace {
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;
int inference_count = 0;

// constexpr int kTensorArenaSize = 248 * 1024; // 调整为 256 KB，足以容纳 200224 字节
// uint8_t *tensor_arena = nullptr;             // 改为指针，从 PSRAM 动态分配

// tensor_arena 放 SRAM1，16字节对齐
constexpr int kTensorArenaSize = 248 * 1024;
__attribute__((section(".sram1"))) alignas(16) static uint8_t tensor_arena[kTensorArenaSize];
} // namespace

// Resize（最近邻插值）并转换 RGB888 一维数组 并且预处理
void pre_process(const uint16_t *input, int in_w, int in_h, int8_t *output, int out_w, int out_h)
{
    mutex_enter_blocking(&g_image_buffer_lock);
    // 遍历输出图像像素
    for (int y = 0; y < out_h; y++) {
        for (int x = 0; x < out_w; x++) {
            // 最近邻插值选取输入像素
            int src_x = x * in_w / out_w;
            int src_y = y * in_h / out_h;

            uint16_t pixel = input[src_y * in_w + src_x];

            uint8_t r, g, b;
            rgb565_to_rgb888(pixel, &r, &g, &b);

            // 写入 int8 输出张量，中心化到 [-128,127]
            int dst_idx = (y * out_w + x) * 3;
            output[dst_idx + 0] = (int8_t)(r - 128); // 减去128以适应int8量化范围
            output[dst_idx + 1] = (int8_t)(g - 128); // 减去128以适应int8量化范围
            output[dst_idx + 2] = (int8_t)(b - 128); // 减去128以适应int8量化范围
        }
    }
    mutex_exit(&g_image_buffer_lock);
}

// 模型输出后处理：解析 bbox + NMS
void post_process(int8_t *raw_data, BBox *boxes, int *box_count)
{
    printf("post_process... \r\n");
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

    // 打印调试
    printf("Detected %d objects:\r\n", *box_count);
    for (int i = 0; i < *box_count; i++) {
        printf("Object %d: x1=%.2f, y1=%.2f, x2=%.2f, y2=%.2f, conf=%.2f, class=%d\r\n", i, boxes[i].x1, boxes[i].y1,
               boxes[i].x2, boxes[i].y2, boxes[i].conf, boxes[i].class_id);
    }
}

// 初始化 PICO YOLO 模型
void pico_yolo_init()
{
    // tensor_arena = (uint8_t *)malloc(kTensorArenaSize);
    // if (tensor_arena == nullptr)
    // {
    //   MicroPrintf("Failed to allocate tensor_arena in PSRAM");
    //   return;
    // }
    // MicroPrintf("Allocated %d bytes in PSRAM for tensor_arena", kTensorArenaSize);

    // 加载模型
    model = tflite::GetModel(Fire_int8_weight);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        MicroPrintf("Model provided is schema version %d not equal "
                    "to supported version %d.",
                    model->version(), TFLITE_SCHEMA_VERSION);
        return;
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
        return;
    }

    // 获取输入输出张量
    input = interpreter->input(0);
    output = interpreter->output(0);

    inference_count = 0;
}

// 运行一次推理 (含耗时统计)
void pico_yolo_infer(const uint16_t *RGB565_input, BBox *boxes, int *box_count)
{
    // 计时开始
    uint64_t start_time_total = time_us_64();

    // 预处理计时
    uint64_t start_time_pre = time_us_64();
    // Step1: 预处理: 执行 resize + RGB888转换 + pre_process
    pre_process(RGB565_input, 320, 240, input->data.int8, 192, 192);

    uint64_t end_time_pre = time_us_64();
    uint64_t pre_process_time_us = end_time_pre - start_time_pre;
    uint64_t pre_process_time_ms = pre_process_time_us / 1000;

    // 模型推理计时
    uint64_t start_time_infer = time_us_64();
    // Step2: 模型推理
    TfLiteStatus invoke_status = interpreter->Invoke();

    uint64_t end_time_infer = time_us_64();
    uint64_t infer_time_us = end_time_infer - start_time_infer;
    uint64_t infer_time_ms = infer_time_us / 1000;

    if (invoke_status != kTfLiteOk) {
        MicroPrintf("Invoke failed!");
        return;
    }

    // 后处理计时
    uint64_t start_time_post = time_us_64();
    post_process(output->data.int8, boxes, box_count);
    uint64_t end_time_post = time_us_64();
    uint64_t post_process_time_us = end_time_post - start_time_post;
    uint64_t post_process_time_ms = post_process_time_us / 1000;

    // 总耗时
    uint64_t end_time_total = time_us_64();
    uint64_t total_time_us = end_time_total - start_time_total;
    uint64_t total_time_ms = total_time_us / 1000;

    // 打印各阶段耗时（微秒和毫秒）
    printf("------[ Timing Breakdown ]------\n");
    printf("Pre-processing time: %llu us (%llu ms)\n", pre_process_time_us, pre_process_time_ms);
    printf("Inference time: %llu us (%llu ms)\n", infer_time_us, infer_time_ms);
    printf("Post-processing time: %llu us (%llu ms)\n", post_process_time_us, post_process_time_ms);
    printf("Total time spent: %llu us (%llu ms)\n", total_time_us, total_time_ms);
    printf("inference done !\n");
    printf("------------------------------------------------------------------\n");
}
