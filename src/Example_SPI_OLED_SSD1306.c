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
#include "lpc8xx_syscon.h"
#include "lpc8xx_gpio.h"
#include "lpc8xx_swm.h"

#include "ssd1306.h"
#include "delay.h"
#include "gfx.h"
#include "testers/gfx_tester.h"

#include "adc_dma.h"
#include "quadrature.h"

#define LED_PIN		(P0_16)

/*
 Pins used in this application:

 OLED Display
 LPC    ARD   Description
 -----  ---   -----------
 P0.24	D13	  OLED SCK
 P0.26	D11	  OLED Data/MOSI
 P0.15	D10	  OLED CS
 P0.08	D8	  OLED Reset
 P0.17	D7	  OLED Data/Command Select

 QUADRATURE DECODER
 LPC    ARD   Description
 -----  ---   -----------
 P0.19	D2	  SCT Quad Clock Out
 P0.20	A5	  SCT Quad PHA
 P0.21	A4	  SCT Quad PHB
 P0.22	A3	  SCT Quad Dir Indicator
 P0.26	D11	  SCT Quad Test Synchro Pulse (Conflict with OLED but not used in production)
 P0.27	D9	  SCT Quad ISR Indicator (BLUE LED)

 DMA ADC
 LPC    ARD   Description
 -----  ---   -----------
 P0.23	A2	  DMA based ADC3

 LEDS
 LPC    ARD   Description
 -----  ---   -----------
 P0.16	--	  Debug LED (GREEN)
*/

int main(void)
{
	SystemCoreClockUpdate();

	// Initialize the delay timer (systick) with 1ms intervals
	// use systick to trigger ADC, enable later when using other hw timer
	//delay_init(12000000 / 1000);

	// Reset and enable the GPIO module (peripherals_lib)
	GPIOInit();

	// Initialize the status LED
	LPC_GPIO_PORT->SET0 = (1 << LED_PIN);
	LPC_GPIO_PORT->DIR0 |= (1 << LED_PIN);

	// Initialize the SCT based quadrature decoder
	quadrature_init();

	// Initialize the DMA and systick based ADC sampler
	adc_dma_init();
	adc_dma_set_rate(100); // Set the ADC sample rate in microseconds

	// Start sampling
	adc_dma_start();

	// Stop sampling
	//adc_dma_stop();

    // Initialize the SSD1306 display
	//ssd1306_init();

	// Perform some GFX tests
	//while(1)
	//{
	//    gfx_tester_run();
	//}

	// Blinky Loop
	while(1)
	{
		LPC_GPIO_PORT->CLR0 = (1 << LED_PIN);
		//delay_ms(1000);

		LPC_GPIO_PORT->SET0 = (1 << LED_PIN);
		//delay_ms(1000);
	}

	return 0;
}
