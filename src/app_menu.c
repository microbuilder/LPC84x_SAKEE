/*
===============================================================================
 Name        : app_menu.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Main menu
===============================================================================
 */

#include "LPC8xx.h"
#include "gpio.h"

#include "config.h"
#include "delay.h"
#include "button.h"
#include "qei.h"
#include "ssd1306.h"
#include "gfx.h"
#include "app_menu.h"

static int32_t _app_menu_selected = APP_MENU_OPTION_ABOUT;

void app_menu_render();

void app_menu_init(void)
{
	ssd1306_init();		// Reset the display
    button_init();
}

void app_menu_render()
{
	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-54, 0, 1, "MAIN MENU", 1);	// 54 pixels wide

    ssd1306_set_text(10, 12, 1, "ABOUT", 1);
    ssd1306_set_text(10, 20, 1, "VOLTMETER", 1);
    ssd1306_set_text(10, 28, 1, "OSCILLOSCOPE", 1);
    ssd1306_set_text(10, 36, 1, "I2C BUS SCANNER", 1);
    ssd1306_set_text(10, 44, 1, "WAVEGEN", 1);
    ssd1306_set_text(10, 52, 1, "CONTINUITY TESTER", 1);

    // Draw the selection indicator
    uint8_t y = ((_app_menu_selected + 2) * 8) -2;
    ssd1306_set_pixel(6, y, 1);
    ssd1306_set_pixel(5, y-1, 1);
    ssd1306_set_pixel(5, y, 1);
    ssd1306_set_pixel(5, y+1, 1);
    ssd1306_set_pixel(4, y-2, 1);
    ssd1306_set_pixel(4, y-1, 1);
    ssd1306_set_pixel(4, y, 1);
    ssd1306_set_pixel(4, y+1, 1);
    ssd1306_set_pixel(4, y+2, 1);

    ssd1306_refresh();
}

app_menu_option_t app_menu_run(void)
{
	// Reset the QEI encoder position counter
	int32_t last_position_qei = 0;
	qei_reset_step();
	_app_menu_selected = APP_MENU_OPTION_ABOUT;
    app_menu_render();

    // Wait for the button to execute the selected sub-app
	while (!(button_pressed() &  ( 1 << QEI_SW_PIN)))
    {
		// Check for a scroll request on the QEI
		int32_t abs = qei_abs_step();
		if (abs != last_position_qei)
		{
			_app_menu_selected += qei_offset_step();

			if (_app_menu_selected < 0)
			{
				_app_menu_selected = APP_MENU_OPTION_LAST - 1;
			}
			if (_app_menu_selected >= APP_MENU_OPTION_LAST)
			{
				_app_menu_selected = APP_MENU_OPTION_ABOUT;
			}

			// Track the position
			last_position_qei = abs;

			// Update the display
			app_menu_render();
		}

    	delay_ms(1);
    }

    return _app_menu_selected;
}
