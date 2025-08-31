
#ifndef _LCD_CONFIG_H_
#define _LCD_CONFIG_H_

// SCL 由 SPI0 确定, 目前为 18
// SDA 由 SPI0 确定, 目前为 19
#define  LCD_CS     17  // CS 引脚
#define  LCD_DC     16  // DC 引脚
#define  LCD_RST    20  // RST引脚
#define  LCD_BL     15  // BL 引脚

#define CONFIG_LCD_WIDTH    240
#define CONFIG_LCD_HEIGHT   320


#define CONFIG_IMAGE_SIZE   153600  // 240 * 320 * 2 = 153600

#endif
