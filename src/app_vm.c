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

	// Analog front end setup
	GPIOSetDir(AN_IN_VREF_3_3V_0_971V/32, AN_IN_VREF_3_3V_0_971V%32, 1); /* 3.3V or 0.971V VRef (240K + 100K divider) */
	GPIOSetDir(AN_IN_VDIV_0_787X/32, AN_IN_VDIV_0_787X%32, 1); 			 /* 0.787X voltage divider bypass */
	GPIOSetDir(AN_IN_2_2PF_BLOCKING/32, AN_IN_2_2PF_BLOCKING%32, 1); 	 /* 2.2pF inline AC/DC blocking cap bypass */

	// Disable 0.971V VRef (3.3V by default)
	if (AN_IN_VREF_3_3V_0_971V/32)
	{
		LPC_GPIO_PORT->SET1 = (1 << (AN_IN_VREF_3_3V_0_971V%32));
	}
	else
	{
		LPC_GPIO_PORT->SET0 = (1 << (AN_IN_VREF_3_3V_0_971V%32));
	}

	// Disable 0.787X (27K+100K) Voltage Divider (Full 3.3V input range by default)
	if (AN_IN_VDIV_0_787X/32)
	{
		LPC_GPIO_PORT->SET1 = (1 << (AN_IN_VDIV_0_787X%32));
	}
	else
	{
		LPC_GPIO_PORT->SET0 = (1 << (AN_IN_VDIV_0_787X%32));
	}

	// Disable 2.2pF AC/DC blocking cap (DC coupling by default)
	if (AN_IN_2_2PF_BLOCKING/32)
	{
		LPC_GPIO_PORT->SET1 = (1 << (AN_IN_2_2PF_BLOCKING%32));
	}
	else
	{
		LPC_GPIO_PORT->SET0 = (1 << (AN_IN_2_2PF_BLOCKING%32));
	}

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-54, 0, 1, "VOLTMETER", 1);	// 54 pixels wide

    ssd1306_set_text(90, 32, 1, "mVOLTS", 1);

    // Render the bottom button options
	//ssd1306_fill_rect(0, 55, 127, 8, 1);
    //ssd1306_set_text(8, 56, 0, "HOME", 1);
    //ssd1306_set_text(36, 56, 0, "", 1);
    //ssd1306_set_text(64, 56, 0, "UNIT", 1);
    //ssd1306_set_text(96, 56, 0, "STOP", 1);

	ssd1306_set_text(16, 55, 1, "CLICK FOR MAIN MENU", 1);

    ssd1306_refresh();
}

void app_vm_run(void)
{
	/* Wait for the QEI switch to exit */
	while (!(button_pressed() &  ( 1 << QEI_SW_PIN)))
	{
		uint16_t v = adc_poll_read(ADC_CHANNEL);
	    ssd1306_fill_rect(20, 20, 64, 24, 0);
	    gfx_printdec(20, 20, (int32_t)(v*MV_PER_LSB), 3, 1);
		ssd1306_refresh();
	}
}
