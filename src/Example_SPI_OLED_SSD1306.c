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
#include "app_about.h"
#include "app_vm.h"
#include "app_scope.h"
#include "app_i2cscan.h"
#include "app_wavegen.h"
#include "app_cont.h"

/*
 Pins used in this application:

 OLED Display
 LPC    ARD   Description
 -----  ---   -----------
 P0.6   D13   OLED SCK
 P1.19  D11   OLED Data/MOSI
 P1.18  D10   OLED CS
 P0.16  D8    OLED Reset
 P0.1   D7    OLED Data/Command Select

 SCT QEI and ADC
 LPC    ARD   Description
 -----  ---   -----------
 P0.19  A2	  QEI0 switch/button
 P0.20  D3    SCT_IN0: QEI0 phA (internal pullup enabled)
 P0.21  D2    SCT_IN1: QEI0 phB (internal pullup enabled)

 SCOPE/VOLTMETER ADC
 LPC    ARD   Description
 -----  ---   -----------
 P1.1         AC/DC coupling selection (220nF inline blocking cap)
 P1.2         0.787X voltage divider enable (27K+100K)
 P1.3         3.3V or 0.971V VRef selection (240K+100K divider) -- NOT USED, 3.3V ONLY!
 P0.14  A0    Analog Input (ADC2)

 I2C BUS SCANNER
 LPC    ARD   Description
 -----  ---   -----------
 P0.10  D15   I2C0 SCL
 P0.11  D14   I2C0 SDA

 WAVEGEN DAC OUTPUT
 LPC    ARD   Description
 -----  ---   -----------
 P0.18        DAC1 Enable (Analog Switch)
 P0.29  A5    DAC1 Output

 CONTINUITY TESTER
 LPC    ARD   Description
 -----  ---   -----------
 P0.17  A4    DAC0 Output
 P0.23        Analog Input (ADC3)

 LEDS
 LPC    ARD   Description
 -----  ---   -----------
 P0.0  	  	  Debug LED (BLUE)

 UART (MBED Interface)
 LPC    ARD   Description
 -----  ---   -----------
 P0.24        DEBUG UART RX
 P0.25        DEBUG UART TX

 CAPACITIVE TOUCH BUTTONS
 LPC          Description
 -----  ---   -----------
 P0.31        X0
 P1.0         X1
 P1.8         YL
 P1.9         YH
 P0.30        ACMP_I5
 */

void setup_debug_uart(void);	// Serial.c

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
	delay_init(12000000 / 1000);

	// UART init (see Serial.c)
	setup_debug_uart();
	printf("LPC SAKEE\n\r");

	// Reset and enable the GPIO module (peripherals_lib)
	GPIOInit();

	// Initialize the status LED
	GPIOSetDir(LED_PIN/32, LED_PIN%32, 1);
	led_off();

	// Initialize the QEI switch pin and and other input buttons
	button_init();

	// Initialize the quadrature decoder
	qei_init();

	// Initialize the SSD1306 display
	ssd1306_init();
	ssd1306_refresh();

	// Display the main menu
	while (1)
	{
		app_menu_init();
		app_menu_option_t option = app_menu_run();

		// Run the appropriate sub-app
		switch (option)
		{
		case APP_MENU_OPTION_LAST:
		case APP_MENU_OPTION_ABOUT:
			// Display 'About' menu
			app_about_init();
			app_about_run();
			break;
		case APP_MENU_OPTION_VOLTMETER:
			// Init voltmeter
			app_vm_init();
			app_vm_run();
			break;
		case APP_MENU_OPTION_SCOPE:
			// Init scope
			app_scope_init();
			app_scope_run();
			break;
		case APP_MENU_OPTION_I2CSCANNER:
			// Init I2C bus scanner
			app_i2cscan_init();
			app_i2cscan_run();
			break;
		case APP_MENU_OPTION_WAVEGEN:
			// Init wavegen
			app_wavegen_init();
			app_wavegen_run();
			break;
		case APP_MENU_OPTION_CONTINUITY:
			// Init continuity tester
			app_cont_init();
			app_cont_run();
			break;
		}
	}

	return 0;
}

