/*
===============================================================================
 Name        : Example_SPI_OLED_SSD1306.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Program entry point
===============================================================================
*/

#include <cr_section_macros.h>
#include <stdio.h>
#include <string.h>

#include "LPC8xx.h"
#include "core_cm0plus.h"
#include "syscon.h"
#include "gpio.h"
#include "swm.h"

#include "ssd1306.h"
#include "delay.h"
#include "gfx.h"
#include "button.h"
#include "testers/gfx_tester.h"

#include "config.h"
#include "adc_dma.h"
#include "qei.h"

#include "app_menu.h"
#include "app_vm.h"
#include "app_scope.h"

/*
 Pins used in this application:

 OLED Display
 LPC    ARD   Description
 -----  ---   -----------
 P0.6	D13	  OLED SCK
 P1.19	D11	  OLED Data/MOSI
 P1.18	D10	  OLED CS
 P0.16	D8	  OLED Reset
 P0.1	D7	  OLED Data/Command Select

 SCT QEI and ADC
 LPC    ARD   Description
 -----  ---   -----------
 P0.19  A2	  QEI0 switch/button
 P0.20  D3    SCT_IN0: QEI0 phA (internal pullup enabled)
 P0.21  D2    SCT_IN1: QEI0 phB (internal pullup enabled)

 SCOPE/VOLTMETER ADC
 LPC    ARD   Description
 -----  ---   -----------
 P0.14  A0    Analog Input (ADC2)

 LEDS
 LPC    ARD   Description
 -----  ---   -----------
 P0.0  	--	  Debug LED (GREEN)
*/

void led_on(void)
{
  LPC_GPIO_PORT->CLR0 = (1 << LED_PIN);
}

void led_off(void)
{
  LPC_GPIO_PORT->SET0 = (1 << LED_PIN);
}

int main(void)
{
	SystemCoreClockUpdate();

	// Initialize the delay timer (systick) with 1ms intervals
	// use systick to trigger ADC, enable later when using other hw timer
	delay_init(12000000 / 1000);

	// Reset and enable the GPIO module (peripherals_lib)
	GPIOInit();

	// Initialize the status LED
	GPIOSetDir(LED_PIN/32, LED_PIN%32, 1);
	led_off();

	// Initialize the QEI switch
	button_init();

	// Initialize the SCT based quadrature decoder
	qei_init();

	// Initialize the SSD1306 display
	ssd1306_init();
	ssd1306_refresh();

	// Display the main menu
	app_menu_init();
	while (1)
	{
		app_menu_option_t option = app_menu_wait();
		switch (option)
		{
		case APP_MENU_OPTION_ABOUT:
			break;
		case APP_MENU_OPTION_VOLTMETER:
			// Init voltmeter
			app_vm_init();
			while(1)
			{
				app_vm_refresh();
			}
			break;
		case APP_MENU_OPTION_SCOPE:
			// Init scope at 100kHz
			app_scope_init(APP_SCOPE_RATE_100_KHZ);
			while(1)
			{
				app_scope_run();
			}
			break;
		}
	}

	return 0;
}

