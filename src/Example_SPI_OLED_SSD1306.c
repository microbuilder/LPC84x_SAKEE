/*
===============================================================================
 Name        : Example_SPI_OLED_SSD1306.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
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

int main(void)
{
  SystemCoreClockUpdate();

  // Initialize the delay timer (systick) with 1ms intervals
  // use systick to trigger ADC, enable later when using other hw timer
//  delay_init(12000000 / 1000);

  // Reset the GPIO module and enable its clock (peripherals_lib)
  GPIOInit();

    // Initialize the status LED
	LPC_GPIO_PORT->SET0 = (1 << LED_PIN);
	LPC_GPIO_PORT->DIR0 |= (1 << LED_PIN);

	quadrature_init();

	// Initialize the ADC
	adc_dma_init();
	adc_dma_set_rate(100); // sample rate in microseconds

    // Initialize the SSD1306 display
//    ssd1306_init();
//
//    // Perform some GFX tests
//    while(1)
//    {
//        gfx_tester_run();
//    }

	// Blinky Loop
	while(1)
	{
//	  LPC_GPIO_PORT->CLR0 = (1 << LED_PIN);
//	  delay_ms(1000);
//
//	  LPC_GPIO_PORT->SET0 = (1 << LED_PIN);
//	  delay_ms(1000);
	}

	return 0;
}
