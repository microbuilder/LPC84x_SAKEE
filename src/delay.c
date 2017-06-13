/*
===============================================================================
 Name        : delay.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
*/

#include "LPC8xx.h"
#include "core_cm0plus.h"
#include "syscon.h"
#include "utilities.h"

#include "delay.h"

// Systick is currently used to trigger ADC, enable later using another hw timer
#if 0

volatile uint32_t g_delay_ms_ticks = 0;

void
SysTick_Handler(void)
{
	g_delay_ms_ticks++;
}

void
delay_ms(uint32_t delayms)
{
	uint32_t curTicks;

	curTicks = g_delay_ms_ticks;

	while ((g_delay_ms_ticks - curTicks) < delayms)
	{
//		__WFI();
	}
}

int
delay_init(uint32_t interval)
{
	NVIC_EnableIRQ(SysTick_IRQn);
	return SysTick_Config(interval);
}
#endif
