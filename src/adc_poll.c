/*
===============================================================================
 Name        : adc_poll.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Polling based ADC sampler
===============================================================================
*/

#include "LPC8xx.h"
#include "core_cm0plus.h"
#include "syscon.h"
#include "swm.h"
#include "adc.h"

#include "adc_poll.h"

/*
 Pins used in this application:

 P0.14 [I] - ADC2
*/

int adc_poll_init(void)
{
	LPC_SYSCON->SYSAHBCLKCTRL0 |= (IOCON);  // Enable the IOCON clock

	// Configures ADC2 (P0_14) on the LPC845,
	// which corresponds to pin A0 on the Arduino headers.
	// Configure the SWM (see utilities_lib and lpc8xx_swm.h)
	LPC_SWM->PINENABLE0 &= ~(ADC_2);		// Enable ADC2 on P0_14
	LPC_IOCON->PIO0_14 = 1<<7;	 			// No pull-up/down

	// Configures ADC3 (P0_23) on the LPC845
	LPC_SWM->PINENABLE0 &= ~(ADC_3);		// Enable ADC3 on P0_23
	LPC_IOCON->PIO0_23 = 1<<7;	 			// No pull-up/down

	// ADC initialization
	LPC_SYSCON->PDRUNCFG &= ~(ADC_PD);		// Power up the ADC
	LPC_SYSCON->SYSAHBCLKCTRL0 |= (ADC);    // Enable the ADC
	LPC_SYSCON->PRESETCTRL0 &= (ADC_RST_N);	// Reset the ADC
	LPC_SYSCON->PRESETCTRL0 |= ~(ADC_RST_N);

	// Start the self-calibration
	LPC_ADC->CTRL = 1     << ADC_CALMODE  |	// Calibration mode
			        0     << ADC_LPWRMODE | // Not low power mode
					(1-1) << ADC_CLKDIV;	// @ 12MHz CLKDIV = 1, ADC=500kHz

	// Poll the calibration mode bit until it is cleared
	while ( LPC_ADC->CTRL & (1 << ADC_CALMODE) ) { }

	// Configure clock div
	LPC_ADC->CTRL = (0 << ADC_CALMODE)  |
				  (0 << ADC_LPWRMODE) |
				  (0 << ADC_CLKDIV);

	// Configure the ADC for the appropriate analog supply voltage using the TRM register
	// For a sampling rate higher than 1 Msamples/s, VDDA must be higher than 2.7 V (on the Max board it is 3.3 V)
	LPC_ADC->TRM &= ~(1<<ADC_VRANGE); // '0' for high voltage

	// Exit calibration mode
	LPC_ADC->CTRL = 0     << ADC_CALMODE  |	// Disable calibration mode
			        0     << ADC_LPWRMODE |	// Not low power mode
					(1-1) << ADC_CLKDIV;	// & 12MHz CLKDIV = 1, ADC=~500kHz

	// Write the sequence control word with enable bit set for both sequences
	LPC_ADC->SEQA_CTRL = 0x00;
	LPC_ADC->SEQB_CTRL = 0x00;

	return 0;
}

uint16_t adc_poll_read(uint8_t adc_ch)
{
	uint32_t sample;

	LPC_ADC->SEQA_CTRL = 1UL << ADC_SEQ_ENA |	// Enable sequence
						 0   << ADC_BURST   |   // Single read
						 0   << ADC_START   |   // Do not start
						 1   << ADC_TRIGPOL |   // Trigger pos edge
						 0   << ADC_TRIGGER |   // SW trigger
						 1   << adc_ch;         // Select channel

	// Clear DATAVALID before the next sample
	do
	{
	   sample = LPC_ADC->SEQA_GDAT;
	}
	while((sample & (1UL<<31)) != 0);

	LPC_ADC->SEQA_CTRL = 1UL << ADC_SEQ_ENA | 	// Enable sequence
						 0   << ADC_BURST   | 	// Single read
						 1   << ADC_START   | 	// Start
						 1   << ADC_TRIGPOL | 	// Trigger pos edge
						 0   << ADC_TRIGGER | 	// SW trigger
						 1   << adc_ch;       	// Select channel

	// Wait for DATAVALID = 1
	do
	{
	   sample = LPC_ADC->SEQA_GDAT;
	}
	while((sample & (1UL<<31)) == 0);

	return (uint16_t)((sample & 0xFFF0) >> 4) ;
}
