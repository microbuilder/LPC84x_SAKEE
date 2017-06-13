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
#include "testers/gfx_tester.h"

#include "adc_dma.h"
#include "quadrature.h"

#define LED_PIN		(P0_0)

/*
 Pins used in this application:

 OLED Display
 LPC    ARD   Description
 -----  ---   -----------
 P0.6	  D13	  OLED SCK
 P1.19	D11	  OLED Data/MOSI
 P1.18	D10	  OLED CS
 P0.16	D8	  OLED Reset
 P0.1	  D7	  OLED Data/Command Select

 SCT QEI and ADC
 LPC    ARD   Description
 -----  ---   -----------
 P0.20  D3    SCT_IN0: QEI0 phA
 P0.21  D2    SCT_IN1: QEI0 phB
 P1.21  D4	  Optional: CLKOUT (can be disabled by setting SCT_ADC_DEBUG to 0)

 DMA ADC
 LPC    ARD   Description
 -----  ---   -----------
 P0.14  A0    Analog Input - ADC2
 P0.23  A1    Optional: SCT_OUT3 - ADC Sample Trigger (can be disabled by setting SCT_ADC_DEBUG to 0)

 LEDS
 LPC    ARD   Description
 -----  ---   -----------
 P0.0  	--	  Debug LED (GREEN)


 QUADRATURE DECODER (obsolete)
 LPC    ARD   Description
 -----  ---   -----------
 P0.19	D2	  SCT Quad Clock Out
 P0.20	A5	  SCT Quad PHA
 P0.21	A4	  SCT Quad PHB
 P0.22	A3	  SCT Quad Dir Indicator
 P0.26	D11	  SCT Quad Test Synchro Pulse (Conflict with OLED but not used in production)
 P0.27	D9	  SCT Quad ISR Indicator (BLUE LED)
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
