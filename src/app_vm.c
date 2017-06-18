/*
===============================================================================
 Name        : app_vm.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Basic voltmeter app
===============================================================================
 */

#include "LPC8xx.h"
#include "gpio.h"

#include "config.h"
#include "delay.h"
#include "adc_poll.h"
#include "button.h"
#include "gfx.h"
#include "app_vm.h"

void app_vm_init(void)
{
	adc_poll_init();

	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-54, 0, 1, "VOLTMETER", 1);	// 54 pixels wide

    // Render the bottom button options
	//ssd1306_fill_rect(0, 55, 127, 8, 1);
    //ssd1306_set_text(8, 56, 0, "HOME", 1);
    //ssd1306_set_text(36, 56, 0, "", 1);
    //ssd1306_set_text(64, 56, 0, "UNIT", 1);
    //ssd1306_set_text(96, 56, 0, "STOP", 1);
    //ssd1306_set_text(90, 32, 1, "mVOLTS", 1);

    ssd1306_refresh();
}

void app_vm_refresh(void)
{
	uint16_t v = adc_poll_read(ADC_CHANNEL);
    ssd1306_fill_rect(20, 20, 64, 24, 0);
    gfx_printdec(20, 20, (int32_t)(v*MV_PER_LSB), 3, 1);
	ssd1306_refresh();
}
