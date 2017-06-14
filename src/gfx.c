/*
===============================================================================
 Name        : gfx.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Basic graphics/drawing routines for 128x64 bitmap displays
===============================================================================
*/


#include "gfx.h"
#include "ssd1306.h"

int gfx_waveform_64_32(uint8_t x, uint8_t y, uint8_t color, const uint16_t *wform, uint8_t offset, uint8_t rshift)
{
	uint8_t i,o;

	// Make sure the offset is in range
	if (offset >= 64)
	{
		return 1;
	}

	// Render a 64 sample 10-bit waveform into a 64x32 window
	for (i=0; i<64; i++)
	{
		// Calculate the offset in the lookup table if requested
		o = offset ? (i + offset) % 64 : i;
		// (12-bit data/32 pixels = 128 lsbs per pixel)
		ssd1306_set_pixel(x+i,y+32-(wform[o]>>rshift)/128 , color);
	}

	return 0;
}

int gfx_graticule(uint8_t x, uint8_t y, gfx_graticule_cfg_t *cfg, uint8_t color)
{
	uint8_t cx, cy;
	uint8_t sx, sy;

	cx = x + cfg->w/2;
	cy = y + cfg->h/2;

	// Validate the cfg data
	if (cfg->line_spacing < 1)
	{
		return 1;
	}
	if (cfg->block_spacing < 1)
	{
		return 2;
	}
	if ((cfg->w < 1) || (cfg->w > 128))
	{
		return 3;
	}
	if ((cfg->h < 1) || (cfg->h > 64))
	{
		return 4;
	}

	// Draw a cross in the center of the graticule
	ssd1306_set_pixel(cx-1, cy, color);
	ssd1306_set_pixel(cx, cy, color);
	ssd1306_set_pixel(cx+1, cy, color);
	ssd1306_set_pixel(cx, cy-1, color);
	ssd1306_set_pixel(cx, cy+1, color);

	// Render the grid (one dot every 8 pixels)
	for (sy=0; sy<cfg->h+1; sy++)
	{
		for (sx=0; sx<cfg->w+1; sx++)
		{
			if ((sx % cfg->block_spacing == 0) &&
				(sy % cfg->block_spacing == 0))
			{
				ssd1306_set_pixel(x + sx, y + sy, 1);
			}
		}
	}

	// Render the divider lines if requested
	if (cfg->lines)
	{
		// Render the horizontal center line if requested
		if (cfg->lines & GFX_GRATICULE_LINES_HOR)
		{
			for (sx=0; sx<cfg->w+1; sx++)
			{
				if (sx % cfg->line_spacing == 0)
				{
					ssd1306_set_pixel(x + sx, cy, 1);
				}
			}
		}

		// Render the vertical center line if requested
		if (cfg->lines & GFX_GRATICULE_LINES_VER)
		{
			for (sy=0; sy<cfg->h+1; sy++)
			{
				if (sy % cfg->line_spacing == 0)
				{
					ssd1306_set_pixel(cx, y + sy, 1);
				}
			}
		}

		// Render the top line if requested
		if (cfg->lines & GFX_GRATICULE_LINES_TOP)
		{
			for (sx=0; sx<cfg->w+1; sx++)
			{
				if (sx % cfg->line_spacing == 0)
				{
					ssd1306_set_pixel(x + sx, y, 1);
				}
			}
		}

		// Render the bottom line if requested
		if (cfg->lines & GFX_GRATICULE_LINES_BOT)
		{
			for (sx=0; sx<cfg->w+1; sx++)
			{
				if (sx % cfg->line_spacing == 0)
				{
					ssd1306_set_pixel(x + sx, y+cfg->h, 1);
				}
			}
		}
	}

	return 0;
}

// Converts a single hex value to it's char equivalent
static char gfx_4bhex_to_char(uint8_t hex)
{
	char c;

	// Convert a 4-bit value (0x0..0xF) to it's char equivalent
	// Anything > 4-bits will return '?'
	switch (hex)
	{
	case 0:
		c = '0';
		break;
	case 1:
		c = '1';
		break;
	case 2:
		c = '2';
		break;
	case 3:
		c = '3';
		break;
	case 4:
		c = '4';
		break;
	case 5:
		c = '5';
		break;
	case 6:
		c = '6';
		break;
	case 7:
		c = '7';
		break;
	case 8:
		c = '8';
		break;
	case 9:
		c = '9';
		break;
	case 10:
		c = 'A';
		break;
	case 11:
		c = 'B';
		break;
	case 12:
		c = 'C';
		break;
	case 13:
		c = 'D';
		break;
	case 14:
		c = 'E';
		break;
	case 15:
		c = 'F';
		break;
	default:
		c = '?';
		break;
	}

	return c;
}

int gfx_printhex8(uint8_t x, uint8_t y, uint8_t hex, uint8_t scale, uint8_t color)
{
	char c;

	c = gfx_4bhex_to_char((hex & 0xF0)>>4);
    ssd1306_set_text(x, y, 1, (char *)&c, scale);
	c = gfx_4bhex_to_char(hex & 0xF);
    ssd1306_set_text(x+(5*scale), y, 1, (char *)&c, scale);

	return 0;
}

// Converts the decimal value to it's char equivalent
static char gfx_dec1_to_char(uint8_t dec)
{
	char c;

	// Convert a single digit decimal value (0..9) to it's char equivalent
	// Anything > 9 will return '?'
	switch (dec)
	{
	case 0:
		c = '0';
		break;
	case 1:
		c = '1';
		break;
	case 2:
		c = '2';
		break;
	case 3:
		c = '3';
		break;
	case 4:
		c = '4';
		break;
	case 5:
		c = '5';
		break;
	case 6:
		c = '6';
		break;
	case 7:
		c = '7';
		break;
	case 8:
		c = '8';
		break;
	case 9:
		c = '9';
		break;
	default:
		c = '?';
		break;
	}

	return c;
}

// Determines the number of digits in the specified number
static uint8_t gfx_num_digits(uint32_t x)
{
    return (x < 10 ? 1 :
           (x < 100 ? 2 :
           (x < 1000 ? 3 :
           (x < 10000 ? 4 :
           (x < 100000 ? 5 :
           (x < 1000000 ? 6 :
           (x < 10000000 ? 7 :
           (x < 100000000 ? 8 :
           (x < 1000000000 ? 9 :
           10)))))))));
}

// Divides the supplied integer by 10 the specified number of times
static uint8_t gfx_div_ten(uint32_t dec, uint8_t c)
{
	uint8_t i;

	for (i=0; i<c; i++)
	{
		dec/=10;
	}

	// Only return the last digit
	return dec % 10;
}

int gfx_printdec(uint8_t x, uint8_t y, uint32_t dec, uint8_t scale, uint8_t color)
{
	char c[2] = { 0x00, 0x00 };
	uint8_t len, i;

	len = gfx_num_digits(dec);
	for (i=0;i<len;i++)
	{
		c[0] = gfx_dec1_to_char(gfx_div_ten(dec, len-i-1));
	    ssd1306_set_text(x+(5*scale*i), y, 1, c, scale);
	}

	return 0;
}
