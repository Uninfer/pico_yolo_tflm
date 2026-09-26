
#include "utils.h"
#include <stdlib.h>

/**
 * @brief   计算 IoU，使用 x1,y1,x2,y2 格式
 *
 * @param   a               omit
 * @param   b               omit
 * @return  float           omit
 */
float IOU(BBox *a, BBox *b)
{
    // 转换为 x1,y1,x2,y2，并确保 x1 <= x2, y1 <= y2
    float ax1 = a->x1;
    float ay1 = a->y1;
    float ax2 = a->x2;
    float ay2 = a->y2;

    float bx1 = b->x1;
    float by1 = b->y1;
    float bx2 = b->x2;
    float by2 = b->y2;

    // 计算交集宽度和高度
    float inter_w = fmaxf(0.0f, fminf(ax2, bx2) - fmaxf(ax1, bx1));
    float inter_h = fmaxf(0.0f, fminf(ay2, by2) - fmaxf(ay1, by1));
    float inter_area = inter_w * inter_h;

    // 计算并集面积
    float a_area = (ax2 - ax1) * (ay2 - ay1);
    float b_area = (bx2 - bx1) * (by2 - by1);
    float union_area = a_area + b_area - inter_area;

    // 处理边界情况
    if (union_area <= 0.0f) {
        return 0.0f;                                        // 如果并集面积为 0，返回 0
    }

    return inter_area / union_area;
}

// 比较函数用于 qsort，按置信度降序排序
int compare_boxes(const void *a, const void *b)
{
    float conf_a = ((BBox *)a)->conf;
    float conf_b = ((BBox *)b)->conf;
    if (conf_a < conf_b) {
        return 1;
    }
    if (conf_a > conf_b) {
        return -1;
    }
    return 0;
}

/**
 * @brief   非极大值抑制 (NMS)
 *
 * @param   boxes           omit
 * @param   box_count       omit
 */
void NMS(BBox *boxes, int *box_count)
{
    if (*box_count <= 0) {
        return;
    }

    // 1. 按置信度降序排序
    qsort(boxes, *box_count, sizeof(BBox), compare_boxes);

    int keep[*box_count]; // 标记保留的框
    for (int i = 0; i < *box_count; i++) {
        keep[i] = 1;                                        // 初始都保留
    }

    // 2. 遍历所有框
    for (int i = 0; i < *box_count; i++) {
        if (!keep[i]) {                                     // 跳过已抑制的框
            continue;
        }

        for (int j = i + 1; j < *box_count; j++) {
            if (!keep[j]) {
                continue;
            }

            float iou = IOU(&boxes[i], &boxes[j]);
            if (iou > IOU_THRESHOLD) {
                keep[j] = 0; // 抑制 IoU 超过阈值的框
            }
        }
    }

    // 3. 紧凑整理保留框
    int new_count = 0;
    for (int i = 0; i < *box_count; i++) {
        if (keep[i]) {
            boxes[new_count] = boxes[i];
            new_count++;
        }
    }
    *box_count = new_count;
}
