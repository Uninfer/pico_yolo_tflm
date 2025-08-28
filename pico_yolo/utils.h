#ifndef UTILS_H_
#define UTILS_H_

#include <cstdint>
#include <cmath> // for expf
#include <stdio.h>
#include "main_functions.h"

// Expose a C friendly interface for main functions.
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 将 RGB565 格式的颜色转换为 RGB888
 * @param color 输入的 RGB565 颜色值 (uint16_t)
 * @param r 输出的红色分量指针 (0-255)
 * @param g 输出的绿色分量指针 (0-255)
 * @param b 输出的蓝色分量指针 (0-255)
 */
void rgb565_to_rgb888(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief 计算两个矩形框的 IoU (Intersection over Union)
 * @param a 第一个框，采用 (x1,y1,x2,y2) 格式
 * @param b 第二个框，采用 (x1,y1,x2,y2) 格式
 * @return 两个框的 IoU 值 (0.0f ~ 1.0f)
 */
float IOU(BBox *a, BBox *b);

/**
 * @brief 用于 qsort 的比较函数，按置信度降序排序
 * @param a 第一个框指针
 * @param b 第二个框指针
 * @return <0: a < b, 0: 相等, >0: a > b
 */
int compare_boxes(const void *a, const void *b);

/**
 * @brief 对检测框执行非极大值抑制 (NMS)
 * @param boxes 输入/输出，检测框数组
 * @param box_count 输入时为框的数量，输出时为 NMS 后剩余框的数量
 */
void NMS(BBox *boxes, int *box_count);

#ifdef __cplusplus
}
#endif

#endif // UTILS_H_
