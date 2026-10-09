/* SPDX-License-Identifier: Apache-2.0 */
#ifndef PICO_YOLO_PROCESSING_H
#define PICO_YOLO_PROCESSING_H
#include <stdint.h>
#include "pico/time.h"
#include <string.h>
#include "pico_yolo_inference.h"
/* 输入图像和输出张量分开存放，告诉编译器两者不重叠。 */
#define PICO_YOLO_RESTRICT __restrict
#ifdef __cplusplus
#define PICO_YOLO_STATIC_ASSERT static_assert
#else
#define PICO_YOLO_STATIC_ASSERT _Static_assert
#endif
#define MODEL_WIDTH 192U
#define MODEL_HEIGHT 192U
#define RESIZED_WIDTH 144U
#define RESIZED_HEIGHT 192U
#define PAD_LEFT 24U
#define OUTPUT_CHANNELS 6U
#define OUTPUT_ANCHORS 144U
#define CLASS_COUNT 2U
#define MAX_CANDIDATES 64U
#define CONFIDENCE_THRESHOLD 0.40f
#define IOU_THRESHOLD 0.30f

PICO_YOLO_STATIC_ASSERT(PICO_YOLO_FRAME_WIDTH * 3U == RESIZED_WIDTH * 5U,
                        "horizontal resize must remain exactly 5:3");
PICO_YOLO_STATIC_ASSERT(PICO_YOLO_FRAME_HEIGHT * 3U == RESIZED_HEIGHT * 5U,
                        "vertical resize must remain exactly 5:3");
PICO_YOLO_STATIC_ASSERT(PAD_LEFT * 2U + RESIZED_WIDTH == MODEL_WIDTH,
                        "letterbox columns must fill the model input");

static uint8_t expand5(uint16_t value)
{
    return (uint8_t)((value << 3U) | (value >> 2U));
}

static uint8_t expand6(uint16_t value)
{
    return (uint8_t)((value << 2U) | (value >> 4U));
}

static inline void store_rgb565(uint16_t pixel, int8_t *destination)
{
    destination[0] = (int8_t)((int32_t)expand5((pixel >> 11U) & 0x1fU) - 128);
    destination[1] = (int8_t)((int32_t)expand6((pixel >> 5U) & 0x3fU) - 128);
    destination[2] = (int8_t)((int32_t)expand5(pixel & 0x1fU) - 128);
}

static void __not_in_flash_func(preprocess_row)(const uint16_t *PICO_YOLO_RESTRICT source,
                                                int8_t *PICO_YOLO_RESTRICT destination)
{
    /* RGB565 转 int8：RGB 各减 128；补边像素 114 对应 -14。
     * 每 5 个源像素取第 0、1、3 个，得到 3 个目标像素。 */
    uint32_t group;
    memset(destination, (int)(int8_t)-14, PAD_LEFT * 3U);
    destination += PAD_LEFT * 3U;
    for (group = 0U; group < RESIZED_WIDTH / 3U; ++group) {
        store_rgb565(source[0], destination);
        store_rgb565(source[1], destination + 3U);
        store_rgb565(source[3], destination + 6U);
        source += 5U;
        destination += 9U;
    }
    memset(destination, (int)(int8_t)-14, PAD_LEFT * 3U);
}

static void preprocess_rgb565(const uint16_t *PICO_YOLO_RESTRICT source,
                              int8_t *PICO_YOLO_RESTRICT destination)
{
    uint32_t group;
    for (group = 0U; group < RESIZED_HEIGHT / 3U; ++group) {
        preprocess_row(source, destination);
        preprocess_row(source + PICO_YOLO_FRAME_WIDTH, destination + MODEL_WIDTH * 3U);
        preprocess_row(source + 3U * PICO_YOLO_FRAME_WIDTH, destination + 2U * MODEL_WIDTH * 3U);
        source += 5U * PICO_YOLO_FRAME_WIDTH;
        destination += 3U * MODEL_WIDTH * 3U;
    }
}

static float clampf(float value, float maximum)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static float intersection_over_union(const pico_yolo_box_t *left, const pico_yolo_box_t *right)
{
    const float x1 = left->x1 > right->x1 ? left->x1 : right->x1;
    const float y1 = left->y1 > right->y1 ? left->y1 : right->y1;
    const float x2 = left->x2 < right->x2 ? left->x2 : right->x2;
    const float y2 = left->y2 < right->y2 ? left->y2 : right->y2;
    if (x2 <= x1 || y2 <= y1) {
        return 0.0f;
    }
    const float intersection = (x2 - x1) * (y2 - y1);
    const float left_area = (left->x2 - left->x1) * (left->y2 - left->y1);
    const float right_area = (right->x2 - right->x1) * (right->y2 - right->y1);
    const float total = left_area + right_area - intersection;
    if (total <= 0.0f) {
        return 0.0f;
    }
    return intersection / total;
}

static void insert_candidate(pico_yolo_box_t *items, uint32_t *count,
                             const pico_yolo_box_t *candidate)
{
    /* 按置信度从高到低插入，满 64 个时丢弃最低的候选框。 */
    uint32_t position = *count;
    if (position == MAX_CANDIDATES) {
        if (candidate->confidence <= items[position - 1U].confidence) {
            return;
        }
        --position;
    } else {
        ++*count;
    }
    while (position > 0U && candidate->confidence > items[position - 1U].confidence) {
        if (position < MAX_CANDIDATES) {
            items[position] = items[position - 1U];
        }
        --position;
    }
    items[position] = *candidate;
}

static uint32_t pico_yolo_postprocess_float(const float *values, pico_yolo_box_t *detections,
                                            uint32_t capacity)
{
    pico_yolo_box_t candidates[MAX_CANDIDATES];
    uint32_t candidate_count = 0U;
    uint32_t detection_count = 0U;
    uint32_t anchor;

    /* 第一步：筛掉低分框，并把模型坐标还原到摄像头坐标。 */
    for (anchor = 0U; anchor < OUTPUT_ANCHORS; ++anchor) {
        const float center_x = values[anchor];
        const float center_y = values[OUTPUT_ANCHORS + anchor];
        const float width = values[2U * OUTPUT_ANCHORS + anchor];
        const float height = values[3U * OUTPUT_ANCHORS + anchor];
        uint32_t class_id;
        for (class_id = 0U; class_id < CLASS_COUNT; ++class_id) {
            const float confidence = values[(4U + class_id) * OUTPUT_ANCHORS + anchor];
            pico_yolo_box_t candidate;
            if (confidence < CONFIDENCE_THRESHOLD) {
                continue;
            }
            candidate.x1 = clampf((center_x - width * 0.5f - PAD_LEFT) *
                                      ((float)PICO_YOLO_FRAME_WIDTH / RESIZED_WIDTH),
                                  (float)(PICO_YOLO_FRAME_WIDTH - 1U));
            candidate.y1 = clampf((center_y - height * 0.5f) *
                                      ((float)PICO_YOLO_FRAME_HEIGHT / RESIZED_HEIGHT),
                                  (float)(PICO_YOLO_FRAME_HEIGHT - 1U));
            candidate.x2 = clampf((center_x + width * 0.5f - PAD_LEFT) *
                                      ((float)PICO_YOLO_FRAME_WIDTH / RESIZED_WIDTH),
                                  (float)(PICO_YOLO_FRAME_WIDTH - 1U));
            candidate.y2 = clampf((center_y + height * 0.5f) *
                                      ((float)PICO_YOLO_FRAME_HEIGHT / RESIZED_HEIGHT),
                                  (float)(PICO_YOLO_FRAME_HEIGHT - 1U));
            if (candidate.x2 <= candidate.x1 || candidate.y2 <= candidate.y1) {
                continue;
            }
            candidate.confidence = confidence;
            candidate.class_id = (uint8_t)class_id;
            insert_candidate(candidates, &candidate_count, &candidate);
        }
    }

    /* 第二步：按分数选框，同一类别的重叠框只保留高分框（NMS）。 */
    for (anchor = 0U; anchor < candidate_count && detection_count < capacity; ++anchor) {
        uint32_t kept;
        int suppressed = 0;
        for (kept = 0U; kept < detection_count; ++kept) {
            if (candidates[anchor].class_id == detections[kept].class_id &&
                intersection_over_union(&candidates[anchor], &detections[kept]) > IOU_THRESHOLD) {
                suppressed = 1;
                break;
            }
        }
        if (!suppressed) {
            detections[detection_count] = candidates[anchor];
            ++detection_count;
        }
    }
    return detection_count;
}

#endif
