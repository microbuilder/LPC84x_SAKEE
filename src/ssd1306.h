/*
 * ssd1306.h
 *
 *  Created on: Jan 31, 2017
 *      Author: ktown
 */

#include <stdint.h>
#include "font5x7.h"

#ifndef SSD1306_H_
#define SSD1306_H_

#define SSD1306_SETCONTRAST 							(0x81)
#define SSD1306_DISPLAYALLON_RESUME 					(0xA4)
#define SSD1306_DISPLAYALLON 							(0xA5)
#define SSD1306_NORMALDISPLAY 							(0xA6)
#define SSD1306_INVERTDISPLAY 							(0xA7)
#define SSD1306_DISPLAYOFF 								(0xAE)
#define SSD1306_DISPLAYON 								(0xAF)
#define SSD1306_SETDISPLAYOFFSET 						(0xD3)
#define SSD1306_SETCOMPINS 								(0xDA)
#define SSD1306_SETVCOMDETECT 							(0xDB)
#define SSD1306_SETDISPLAYCLOCKDIV 						(0xD5)
#define SSD1306_SETPRECHARGE 							(0xD9)
#define SSD1306_SETMULTIPLEX 							(0xA8)
#define SSD1306_SETLOWCOLUMN 							(0x00)
#define SSD1306_SETHIGHCOLUMN 							(0x10)
#define SSD1306_SETSTARTLINE 							(0x40)
#define SSD1306_MEMORYMODE 								(0x20)
#define SSD1306_COLUMNADDR 								(0x21)
#define SSD1306_PAGEADDR   								(0x22)
#define SSD1306_COMSCANINC 								(0xC0)
#define SSD1306_COMSCANDEC 								(0xC8)
#define SSD1306_SEGREMAP 								(0xA0)
#define SSD1306_CHARGEPUMP 								(0x8D)
#define SSD1306_EXTERNALVCC 							(0x01)
#define SSD1306_SWITCHCAPVCC 							(0x02)
#define SSD1306_ACTIVATE_SCROLL 						(0x2F)
#define SSD1306_DEACTIVATE_SCROLL 						(0x2E)
#define SSD1306_SET_VERTICAL_SCROLL_AREA 				(0xA3)
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 				(0x26)
#define SSD1306_LEFT_HORIZONTAL_SCROLL 					(0x27)
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 	(0x29)
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 	(0x2A)

int ssd1306_init(void);
int ssd1306_refresh(void);
int ssd1306_clear(void);
int ssd1306_fill(uint8_t pattern);
int ssd1306_invert(uint8_t color);
int ssd1306_set_pixel(uint8_t x, uint8_t y, uint8_t color);
int ssd1306_set_text(uint8_t x, uint8_t y, uint8_t color, char* string, uint8_t scale);
int ssd1306_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

#endif /* SSD1306_H_ */