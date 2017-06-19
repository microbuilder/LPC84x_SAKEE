/*
===============================================================================
 Name        : app_about.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : About menu
===============================================================================
 */

#include "LPC8xx.h"
#include "i2c.h"
#include "swm.h"
#include "syscon.h"

#include "button.h"
#include "app_about.h"
#include "gfx.h"

void app_about_init(void)
{
	ssd1306_clear();
    ssd1306_refresh();
}

void app_about_run(void)
{
	ssd1306_clear();
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-60, 0, 1, "ABOUT SAKEE", 1);
	ssd1306_set_text(16, 55, 1, "CLICK FOR MAIN MENU", 1);
    ssd1306_refresh();

	while (!button_pressed())
	{

	}
}


