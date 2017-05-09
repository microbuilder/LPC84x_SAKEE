/*
===============================================================================
 Name        : gfx.h
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
 */

#ifndef GFX_H_
#define GFX_H_

#include <stdint.h>
#include "ssd1306.h"

typedef enum
{
	GFX_GRATICULE_LINES_NONE = 0,			// No borders or divider lines
	GFX_GRATICULE_LINES_HOR  = (1 << 0),	// Center horizontal line
	GFX_GRATICULE_LINES_VER	 = (1 << 1),	// Center vertical line
	GFX_GRATICULE_LINES_TOP	 = (1 << 2),	// Top border
	GFX_GRATICULE_LINES_BOT	 = (1 << 3),	// Bottom border
} gfx_graticule_lines_t;

typedef struct
{
	uint8_t w;
	uint8_t h;
	gfx_graticule_lines_t lines;
	uint8_t line_spacing;			// Space between each dot on the hor/ver/etc dividers
	uint8_t block_spacing;			// Pixels in each divider in the graticule
} gfx_graticule_cfg_t;

int gfx_waveform_64_32(uint8_t x, uint8_t y, uint8_t color, const uint16_t *wform, uint8_t offset);
int gfx_graticule(uint8_t x, uint8_t y, gfx_graticule_cfg_t *cfg, uint8_t color);
int gfx_printhex8(uint8_t x, uint8_t y, uint8_t hex, uint8_t scale, uint8_t color);
int gfx_printdec(uint8_t x, uint8_t y, uint32_t dec, uint8_t scale, uint8_t color);

#endif /* GFX_H_ */
