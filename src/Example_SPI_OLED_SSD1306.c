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

#if 0
	// GFX Tester
	while (1) gfx_tester_run();
#endif

#if 0
	// test QEI
	while(1)
	{
	  static int32_t last_offset = 0;
	  static int32_t btn_count = 0;

	  ssd1306_clear();

	  ssd1306_set_text(8 , 0, 1, "ABS", 2);
	  ssd1306_set_text(60, 0, 1, "OFFSET", 2);

	  // ABS
	  int32_t abs = qei_abs_step();
	  gfx_printdec(8, 16, abs, 2, 1);

	  // Display offset if non-zero
	  int32_t cur_offset = qei_offset_step();
	  if ( cur_offset )
	  {
	    last_offset = cur_offset;
	  }
	  gfx_printdec(60, 16, last_offset, 2, 1);

#if 1
	  // Select button counter
	  btn_count += (button_pressed() ? 1 : 0);
	  ssd1306_set_text(8 , 40, 1, "btn", 2);
	  gfx_printdec(60, 40, btn_count, 2, 1);
#else
	  // A/B pin states
	  ssd1306_set_text(8 , 32, 1, "PA", 2);
	  ssd1306_set_text(60, 32, 1, "PB", 2);

	  uint8_t a_value = (qei_read_pin() & bit(0)) ? 1 : 0;
	  uint8_t b_value = (qei_read_pin() & bit(1)) ? 1 : 0;
	  gfx_printdec(8 , 48, a_value, 2, 1);
	  gfx_printdec(60, 48, b_value, 2, 1);

#endif

	  ssd1306_refresh();

	  delay_ms(1);
	}
#endif

// Voltmeter App
#if 0
	app_vm_init();
	while(1)
	{
		app_vm_refresh();
	}
#endif

// Scope App
#if 1
	app_scope_init(APP_SCOPE_RATE_100_KHZ);
	while(1)
	{
		app_scope_run();
	}
#endif

	return 0;
}

