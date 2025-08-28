#ifndef MAIN_FUNCTIONS_H_
#define MAIN_FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define CONF_THRESHOLD 0.40f ///< 置信度阈值 (分类分数大于此值才保留)
#define IOU_THRESHOLD 0.30f  ///< IOU 阈值 (NMS 中用来抑制重叠框)
#define MAX_BOXES 10         ///< 最大检测框数

/**
 * @brief   检测框结构体
 *
 */
typedef struct {
    float x1, y1, x2, y2, conf;
    int class_id;
} BBox;

/**
 * @brief 输入图像预处理：Resize + RGB565 转 RGB888 + 归一化/量化
 * @param input 输入的 RGB565 格式图像数据 (大小 in_w * in_h)
 * @param in_w 输入图像宽度
 * @param in_h 输入图像高度
 * @param output 输出的 int8 张量 (大小 out_w * out_h * 3)
 * @param out_w 模型输入宽度
 * @param out_h 模型输入高度
 */
void pre_process(const uint16_t *input, int in_w, int in_h, int8_t *output, int out_w, int out_h);

/**
 * @brief 模型推理后的后处理：解析输出、解码边界框、过滤 + NMS
 * @param raw_data 模型输出的 int8 原始数据
 * @param boxes 输出的检测框数组
 * @param box_count 输入时为可容纳最大数量，输出时为实际检测到的数量
 */
void post_process(int8_t *raw_data, BBox *boxes, int *box_count);

/**
 * @brief 初始化 Pico YOLO 模型解释器
 * - 加载 TFLite 模型
 * - 注册算子
 * - 分配张量内存
 * - 获取输入输出张量
 */
void pico_yolo_init();

/**
 * @brief 执行一次推理流程：预处理 -> 模型推理 -> 后处理
 * @param RGB565_input 输入的相机帧 (RGB565 格式, 320x240)
 * @param boxes 输出的检测框数组
 * @param box_count 输入时为可容纳最大数量，输出时为实际检测到的数量
 */
void pico_yolo_infer(const uint16_t *RGB565_input, BBox *boxes, int *box_count);

#ifdef __cplusplus
}
#endif

#endif // MAIN_FUNCTIONS_H_
