/*
===============================================================================
 Name        : app_cont.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Continuity tester
===============================================================================
 */

#include "LPC8xx.h"
#include "i2c.h"
#include "swm.h"
#include "syscon.h"
#include "gpio.h"

#include "adc_poll.h"
#include "config.h"
#include "delay.h"
#include "button.h"
#include "app_cont.h"
#include "gfx.h"

#define CONT_ADC_CHANNEL (3)

void app_cont_init(void)
{
	adc_poll_init();

	// Initialize the status LED
	GPIOSetDir(LED_PIN/32, LED_PIN%32, 1);
	LPC_GPIO_PORT->SET0 = (1 << LED_PIN);

	ssd1306_clear();
    ssd1306_refresh();
}

void app_cont_run(void)
{
	ssd1306_clear();
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-60, 0, 1, "CONT TESTER", 1);
	ssd1306_set_text(8, 24, 1, "CONNECT V1 AND COM FOR", 1);
	ssd1306_set_text(8, 32, 1, "   CONTINUITY TEST", 1);
	ssd1306_set_text(16, 55, 1, "CLICK FOR MAIN MENU", 1);
    ssd1306_refresh();

	/* Wait for the QEI switch to exit */
	while (!(button_pressed() &  ( 1 << QEI_SW_PIN)))
	{
		uint16_t v = adc_poll_read(CONT_ADC_CHANNEL);
		if (v < 100)
		{
			ssd1306_invert(1);
			// Turn the LED on
			LPC_GPIO_PORT->CLR0 = (1 << LED_PIN);
		}
		else
		{
			ssd1306_invert(0);
			// Turn the LED off
			LPC_GPIO_PORT->SET0 = (1 << LED_PIN);
		}
		ssd1306_refresh();
	}
}
