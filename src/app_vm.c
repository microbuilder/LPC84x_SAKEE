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

	/* Analog front end setup */
	GPIOSetDir(AN_IN_VREF_3_3V/32, AN_IN_VREF_3_3V%32, 1);				/* Straight 3.3V VRef */
	GPIOSetDir(AN_IN_VREF_0_971V/32, AN_IN_VREF_0_971V%32, 1);			/* 240K + 100K */
	GPIOSetDir(AN_IN_VREF_DISCON/32, AN_IN_VREF_DISCON%32, 1); 			/* No VREF (NC res) */
	GPIOSetDir(AN_IN_VREF_2_598V/32, AN_IN_VREF_2_598V%32, 1); 			/* 2.598V VRef bypass */
	GPIOSetDir(AN_IN_2_2PF_BLOCKING/32, AN_IN_2_2PF_BLOCKING%32, 1); 	/* 2.pF inline blocking cap bypass */

	// Enable 3.3V VRef
	if (AN_IN_VREF_3_3V/32)
	{
		LPC_GPIO_PORT->SET1 = (1 << (AN_IN_VREF_3_3V%32));
	}
	else
	{
		LPC_GPIO_PORT->SET0 = (1 << (AN_IN_VREF_3_3V%32));
	}

	// Disable 0.971V VRef
	if (AN_IN_VREF_0_971V/32)
	{
		LPC_GPIO_PORT->CLR1 = (1 << (AN_IN_VREF_0_971V%32));
	}
	else
	{
		LPC_GPIO_PORT->CLR0 = (1 << (AN_IN_VREF_0_971V%32));
	}

	// Disable VRef disconnect
	if (AN_IN_VREF_DISCON/32)
	{
		LPC_GPIO_PORT->CLR1 = (1 << (AN_IN_VREF_DISCON%32));
	}
	else
	{
		LPC_GPIO_PORT->CLR0 = (1 << (AN_IN_VREF_DISCON%32));
	}

	// Skip 2.598V (27K+100K) Voltage Divider
	if (AN_IN_VREF_2_598V/32)
	{
		LPC_GPIO_PORT->SET1 = (1 << (AN_IN_VREF_2_598V%32));
	}
	else
	{
		LPC_GPIO_PORT->SET0 = (1 << (AN_IN_VREF_2_598V%32));
	}

	// Disable 2.2pF blocking cap
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
