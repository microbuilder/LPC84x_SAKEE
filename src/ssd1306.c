/*
===============================================================================
 Name        : ssd1306.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : SPI based SSD1306 driver for 128x64 pixel OLED displays
===============================================================================
*/

#include <string.h>

#include "LPC8xx.h"
#include "ssd1306.h"
#include "syscon.h"
#include "spi.h"
#include "swm.h"
#include "gpio.h"
#include "utilities.h"
#include "delay.h"

// Note: The NXP SAKEE board uses a 4-wire SPI interface by default
// BS0, BS1 and BS2 = GND
// In this mode:
// CMD  = D/C: HIGH
// DATA = D/C: LOW

#define SSD1306_DCPIN	(P0_1)	    // D7 	--> OLED Data/Command (was P0_17)
#define SSD1306_RSTPIN	(P0_16)	    // D8	--> OLED Reset (was P0_13)
#define SSD1306_SSELPIN (P1_18)	    // D10	--> OLED CS (was P0_15)
#define SSD1306_MOSIPIN	(P1_19)	    // D11	--> OLED Data/MOSI (was P0_26)
#define SSD1306_SCKPIN	(P0_6)	    // D13	--> OLED Clock/SCK (was P0_24)

#define SSD1306_WIDTH	(128)
#define SSD1306_HEIGHT	(64)

static uint8_t buffer[(SSD1306_WIDTH * SSD1306_HEIGHT) / 8];

static int
ssd1306_command(uint8_t cmd)
{
	LPC_GPIO_PORT->SET1 = (1 << (SSD1306_SSELPIN%32));	// CS HIGH
	LPC_GPIO_PORT->CLR0 = (1 << SSD1306_DCPIN%32);		// DC LOW = Command
	LPC_GPIO_PORT->CLR1 = (1 << (SSD1306_SSELPIN%32));	// CS LOW = Assert
    LPC_SPI1->TXCTL &= ~(CTL_EOT);                      // Start a new transfer, clear the EOT bit
    while ((LPC_SPI1->STAT & STAT_TXRDY) == 0);  	    // Wait for master TXRDY
    LPC_SPI1->TXDAT = cmd;                       	    // Write the cmd byte to the master's TXDAT register, start the frame
    //LPC_SPI1->TXCTL |= CTL_EOT;
	LPC_GPIO_PORT->SET1 = (1 << (SSD1306_SSELPIN%32));	// CS HIGH

	return 0;
}

static int
ssd1306_data(uint8_t data)
{
	LPC_GPIO_PORT->SET1 = (1 << (SSD1306_SSELPIN%32));	// CS HIGH
	LPC_GPIO_PORT->SET0 = (1 << SSD1306_DCPIN%32);		// DC HIGH = Data
	LPC_GPIO_PORT->CLR1 = (1 << (SSD1306_SSELPIN%32));	// CS LOW = Assert
    LPC_SPI1->TXCTL &= ~(CTL_EOT);                      // Start a new transfer, clear the EOT bit
    while ((LPC_SPI1->STAT & STAT_TXRDY) == 0);  	    // Wait for master TXRDY
    LPC_SPI1->TXDAT = data;                       	    // Write the data byte to the master's TXDAT register, start the frame
    while ((LPC_SPI1->STAT & STAT_TXRDY) == 0);	        // Wait for master TXRDY
    //LPC_SPI1->TXCTL |= CTL_EOT;
	LPC_GPIO_PORT->SET1 = (1 << (SSD1306_SSELPIN%32));	// CS HIGH

	return 0;
}

static int
ssd1306_reset(void)
{
	LPC_GPIO_PORT->SET0 = (1 << SSD1306_RSTPIN%32);	// RST HIGH
	delay_ms(1);									// Delay 1ms
	LPC_GPIO_PORT->CLR0 = (1 << SSD1306_RSTPIN%32);	// RST LOW = Assert RST
	delay_ms(10);									// Delay 10ms
	LPC_GPIO_PORT->SET0 = (1 << SSD1306_RSTPIN%32);	// RST HIGH = Release RST

	return 0;
}

static int
ssd1306_pin_setup(void)
{
	// Select the function clock source for the master
	LPC_SYSCON->SPI1CLKSEL = FCLKSEL_MAIN_CLK;

	// Make sure SPI1 and SWM clocks are enabled
	LPC_SYSCON->SYSAHBCLKCTRL0 |= (SPI1 | SWM);

	// Set the GPIO pins to their initial state
	LPC_GPIO_PORT->CLR0 = (1 << SSD1306_DCPIN%32);		// 0
	LPC_GPIO_PORT->SET1 = (1 << (SSD1306_SSELPIN%32));	// 1 = Not asserted
	LPC_GPIO_PORT->SET0 = (1 << SSD1306_RSTPIN%32);	// 1 = Active

	// Set GPIO pins to output
	LPC_GPIO_PORT->DIR0 |= (1 << SSD1306_DCPIN%32);
	LPC_GPIO_PORT->DIR1 |= (1 << (SSD1306_SSELPIN%32));
	LPC_GPIO_PORT->DIR0 |= (1 << SSD1306_RSTPIN%32);

	// Configure the SWM (see utilities_lib and lpc8xx_swm.h)
	ConfigSWM(SPI1_SCK, SSD1306_SCKPIN);
	ConfigSWM(SPI1_MOSI, SSD1306_MOSIPIN);
	ConfigSWM(SPI1_SSEL0, SSD1306_SSELPIN);

	// Reset SPI block
	LPC_SYSCON->PRESETCTRL0 &= (SPI1_RST_N);
	LPC_SYSCON->PRESETCTRL0 |= ~(SPI1_RST_N);

	// Configure the SPI master's clock divider (value written to DIV divides by value+1)
	SystemCoreClockUpdate();                // Get main_clk frequency
	LPC_SPI1->DIV = (main_clk/1000000) - 1;

	// Configure the CFG register:
	// Enable=true, master, no LSB first, CPHA=0, CPOL=0, no loop-back, SSEL active low
	LPC_SPI1->CFG = CFG_ENABLE | CFG_MASTER;

	// Configure the SPI delay register (DLY)
	// Pre-delay = 0 clocks, post-delay = 0 clocks, frame-delay = 0 clocks, transfer-delay = 0 clocks
	LPC_SPI1->DLY = 0x0000;

	// Configure the SPI control register
	// Master: End-of-frame true, End-of-transfer true, RXIGNORE true, LEN 8 bits.
	LPC_SPI1->TXCTL = CTL_EOF  | CTL_EOT | CTL_RXIGNORE | CTL_LEN(8);

	return 0;
}

static int
ssd1306_config_display()
{
	ssd1306_reset();

	// Turn the OLED Display off
	ssd1306_command(SSD1306_DISPLAYOFF);

	// Configure the display for 128x64 pixels, KS0108 mode
	ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);
	ssd1306_command(0x80);
	ssd1306_command(SSD1306_SETMULTIPLEX);
	ssd1306_command(SSD1306_HEIGHT-1);				// LCD Height
	ssd1306_command(SSD1306_SETDISPLAYOFFSET);
	ssd1306_command(0x0);
	ssd1306_command(SSD1306_SETSTARTLINE | 0x0);
	ssd1306_command(SSD1306_CHARGEPUMP);
	ssd1306_command(0x14);							// Use 3.3V supply to generate high voltage supply
	ssd1306_command(SSD1306_MEMORYMODE);
	ssd1306_command(0x00);
	ssd1306_command(SSD1306_SEGREMAP | 0x1);
	ssd1306_command(SSD1306_COMSCANDEC);
	ssd1306_command(SSD1306_SETCOMPINS);
	ssd1306_command(0x12);
	ssd1306_command(SSD1306_SETCONTRAST);
	ssd1306_command(0xCF);
    ssd1306_command(SSD1306_SETPRECHARGE);
    ssd1306_command(0xF1);
	ssd1306_command(SSD1306_SETVCOMDETECT);
	ssd1306_command(0x40);
	ssd1306_command(SSD1306_DISPLAYALLON_RESUME);
	ssd1306_command(SSD1306_NORMALDISPLAY);
	ssd1306_command(SSD1306_DEACTIVATE_SCROLL);

	ssd1306_command(SSD1306_COLUMNADDR);
	ssd1306_command(0);
	ssd1306_command(SSD1306_WIDTH-1);
	ssd1306_command(SSD1306_PAGEADDR);
	ssd1306_command(0);
	ssd1306_command(SSD1306_HEIGHT/8-1);

	// Turn the OLED display on!
	ssd1306_command(SSD1306_DISPLAYON);

    ssd1306_command(SSD1306_SETLOWCOLUMN | 0x0);  // low col = 0
    ssd1306_command(SSD1306_SETHIGHCOLUMN | 0x0);  // hi col = 0
    ssd1306_command(SSD1306_SETSTARTLINE | 0x0); // line #0

	return 0;
}

static int
ssd1306_render_char(uint8_t x, uint8_t y, uint8_t color, char c, uint8_t scale)
{
	uint8_t px, py;
	uint16_t start_pos;

	if ((x >= SSD1306_WIDTH) || (y >= SSD1306_HEIGHT)) {
		return 1;
	}
	if (c > 127) {
		return 2;
	}
	if (scale > 3) {
		return 3;
	}

	start_pos = ((uint8_t)c) * 7;			// Characters have a 7 row offset
	for (px=0; px<5; px++) {
		for (py=0; py<7; py++) {
			if ((font5x7[start_pos+py] >> (7-px)) & 1) {
				switch (scale) {
				case 3:
					ssd1306_set_pixel(x+(px*scale),   y+(py*scale),  color);
					ssd1306_set_pixel(x+(px*scale)+1, y+(py*scale),  color);
					ssd1306_set_pixel(x+(px*scale)+2, y+(py*scale),  color);
					ssd1306_set_pixel(x+(px*scale),   y+(py*scale)+1, color);
					ssd1306_set_pixel(x+(px*scale)+1, y+(py*scale)+1, color);
					ssd1306_set_pixel(x+(px*scale)+2, y+(py*scale)+1, color);
					ssd1306_set_pixel(x+(px*scale),   y+(py*scale)+2, color);
					ssd1306_set_pixel(x+(px*scale)+1, y+(py*scale)+2, color);
					ssd1306_set_pixel(x+(px*scale)+2, y+(py*scale)+2, color);
					break;
				case 2:
					ssd1306_set_pixel(x+(px*scale),   y+(py*scale),  color);
					ssd1306_set_pixel(x+(px*scale)+1, y+(py*scale),  color);
					ssd1306_set_pixel(x+(px*scale),   y+(py*scale)+1, color);
					ssd1306_set_pixel(x+(px*scale)+1, y+(py*scale)+1, color);
					break;
				case 1:
				default:
					ssd1306_set_pixel(x+px, y+py, color);
					break;
				}
			}
		}
	}

	return 0;
}

int
ssd1306_init(void)
{
	// Configure the SPI1 peripheral block
	// Note: This module assumes 'GPIOInit()' has already been called!
	ssd1306_pin_setup();

	// Give the display a reset
	ssd1306_reset();

	// Clear the framebuffer
	memset(buffer, 0, sizeof(buffer));

	// Configure the SSD1306 display controller
	ssd1306_config_display();

	return 0;
}

int
ssd1306_refresh(void)
{
	uint16_t i;

	ssd1306_command(0xb0);
	ssd1306_command(((0&0xf0)>>4)|0x10);
	ssd1306_command((0&0x0f)|0x01);

    for (i=0; i<sizeof(buffer)-1; i++) {
    	ssd1306_data(buffer[i]);
    }

	return 0;
}

int
ssd1306_clear(void)
{
	memset(buffer, 0, sizeof(buffer));

	return 0;
}

int
ssd1306_fill(uint8_t pattern)
{
	memset(buffer, pattern, sizeof(buffer));
	return 0;
}

int
ssd1306_invert(uint8_t color)
{
	if (color) {
		ssd1306_command(SSD1306_INVERTDISPLAY);
	}
	else {
		ssd1306_command(SSD1306_NORMALDISPLAY);
	}

	return 0;
}

int
ssd1306_set_pixel(uint8_t x, uint8_t y, uint8_t color)
{
	if ((x >= SSD1306_WIDTH) || (y >= SSD1306_HEIGHT)) {
		return 1;
	}

    switch (color)
    {
      case 0:
    	  buffer[x + (y/8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
    	  break;
      default:
    	  buffer[x + (y/8) * SSD1306_WIDTH] |=  (1 << (y & 7));
    	  break;
    }

	return 0;
}

int
ssd1306_set_text(uint8_t x, uint8_t y, uint8_t color, char* string, uint8_t scale)
{
	if ((x >= SSD1306_WIDTH) || (y >= SSD1306_HEIGHT)) {
		return 1;
	}

	if (scale > 3) {
		return 2;
	}

	uint16_t i;
	for (i = 0; string[i] != '\0'; i++) {
		// Catch overflow when scaling!
		uint16_t xscaled = x+(i*5*scale);
		if (xscaled > SSD1306_WIDTH) {
			return 0;
		} else {
			ssd1306_render_char(xscaled, y, color, string[i], scale);
		}
	}

	return 0;
}

int ssd1306_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
	uint16_t a,b;

	if ((x >= SSD1306_WIDTH) || (y >= SSD1306_HEIGHT)) {
		return 1;
	}

	// ToDo: Highly inefficient, optimize!
	for (a = 0; a < h; a++)
	{
		for (b = 0; b < w; b++)
		{
			ssd1306_set_pixel(x+b, y+a, color);
		}
	}

	return 0;
}
