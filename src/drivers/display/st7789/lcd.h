
#ifndef _LCD_H_
#define _LCD_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void lcd_init(void);
void lcd_show_image(uint16_t *image);
void draw_boxes_to_lcd(int x1, int y1, int x2, int y2, uint16_t color);

#ifdef __cplusplus
}
#endif

#endif
