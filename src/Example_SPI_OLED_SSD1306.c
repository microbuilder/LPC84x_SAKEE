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

#include "LPC8xx.h"
#include "core_cm0plus.h"
#include "lpc8xx_syscon.h"
#include "lpc8xx_gpio.h"
#include "lpc8xx_swm.h"

#include "ssd1306.h"
#include "delay.h"
#include "adc.h"
#include "gfx.h"
#include "testers/gfx_tester.h"

#include "adc_dma.h"

#define LED_PIN		(P0_16)

extern uint16_t adc_buffer[DMA_BUFFER_SIZE];

int main(void) {
  // Initialize the delay timer (systick) with 1ms intervals
  delay_init(12000000 / 1000);

  // Reset the GPIO module and enable its clock (peripherals_lib)
  GPIOInit();

    // Initialize the status LED
	LPC_GPIO_PORT->CLR0 = (1 << LED_PIN);
	LPC_GPIO_PORT->DIR0 |= (1 << LED_PIN);

	memset(adc_buffer, 0, DMA_BUFFER_SIZE*2);

	// Initialize the ADC
	adc_dma_init();
//	adc_init();

    // Initialize the SSD1306 display
//    ssd1306_init();
//
//    // Perform some GFX tests
//    while(1)
//    {
//        gfx_tester_run();
//    }

	adc_dma_start();

	// wait for DMA0 complete
//	while( !(LPC_DMA->INTA0 & 0x01UL) ) {}

	// Blinky Loop
	while(1)
	{
	  LPC_GPIO_PORT->CLR0 = (1 << LED_PIN);
	  delay_ms(1000);

	  LPC_GPIO_PORT->SET0 = (1 << LED_PIN);
	  delay_ms(1000);
	}

	return 0;
}
