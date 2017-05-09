/*
===============================================================================
 Name        : delay.h
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
*/

#ifndef DELAY_H_
#define DELAY_H_

#include "LPC8xx.h"
#include "core_cm0plus.h"
#include "lpc8xx_syscon.h"
#include "utilities.h"

#define DELAY_SYSTICK_TIME (12000000 - 1)	// default set to .4 second @ 30 MHz (clocked by PLL)

void delay_ms(uint32_t delayms);
int delay_init(uint32_t interval);

#endif /* DELAY_H_ */
