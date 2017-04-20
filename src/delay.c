/*
 * delay.c
 *
 *  Created on: Feb 3, 2017
 *      Author: ktown
 */

#include "LPC8xx.h"
#include "core_cm0plus.h"
#include "lpc8xx_syscon.h"
#include "utilities.h"

#include "delay.h"

volatile uint32_t g_delay_ms_ticks;

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
		__WFI();
	}
}

int
delay_init(uint32_t interval)
{
	NVIC_EnableIRQ(SysTick_IRQn);
	return SysTick_Config(interval);
}
