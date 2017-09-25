/*
===============================================================================
 Name        : app_cont.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Continuity tester
===============================================================================
 */

#include "LPC8xx.h"
#include "i2c.h"
#include "swm.h"
#include "syscon.h"
#include "gpio.h"

#include "adc_poll.h"
#include "config.h"
#include "delay.h"
#include "button.h"
#include "app_cont.h"
#include "gfx.h"
#include "dac_wavegen.h"

#define CONT_ADC_CHANNEL (3)

// 0.0 .. 1.8V exponential decay by default
static const uint16_t app_cont_dac_output[64] = {
	   0,  34,  66,  95, 123, 150, 174, 198,
	 220, 240, 259, 277, 294, 310, 325, 339,
	 353, 365, 377, 388, 398, 408, 417, 425,
	 433, 441, 448, 455, 461, 467, 472, 478,
	 482, 487, 491, 495, 499, 503, 506, 509,
	 512, 515, 518, 520, 522, 524, 527, 528,
	 530, 532, 533, 535, 536, 538, 539, 540,
	 541, 542, 543, 544, 545, 546, 546, 547
};

void app_cont_init(void)
{
	adc_poll_init();

	// Initialize the status LED
	GPIOSetDir(LED_PIN/32, LED_PIN%32, 1);
	LPC_GPIO_PORT->SET0 = (1 << LED_PIN);

	// Initialize the DAC
	dac_wavegen_init(WAVEGEN_DAC);

	// Setup the analog switch that controls speaker/DACOUT
	GPIOSetDir(DAC1EN_PIN/32, DAC1EN_PIN%32, 1);
	LPC_GPIO_PORT->SET0 = (1 << DAC1EN_PIN);	// Default to speaker

	ssd1306_clear();
    ssd1306_refresh();
}

void app_cont_run(void)
{
	ssd1306_clear();
    ssd1306_set_text(0, 0, 1, "LPC SAKEE", 1);
    ssd1306_set_text(127-60, 0, 1, "CONT TESTER", 1);
	ssd1306_set_text(8, 24, 1, "CONNECT V1 AND COM FOR", 1);
	ssd1306_set_text(8, 32, 1, "   CONTINUITY TEST", 1);
	ssd1306_set_text(16, 55, 1, "CLICK FOR MAIN MENU", 1);
    ssd1306_refresh();

	// Enable the DAC output for an audible alert
	dac_wavegen_run(WAVEGEN_DAC, app_cont_dac_output, sizeof(app_cont_dac_output)/2, 500);
    GPIOSetDir(AUDIO_AMP_ENABLE_PORT, AUDIO_AMP_ENABLE_PIN, OUTPUT);
    // Disable the audio output by default
    GPIOSetBitValue(AUDIO_AMP_ENABLE_PORT, AUDIO_AMP_ENABLE_PIN, 0);

	/* Wait for the QEI switch to exit */
	while (!(button_pressed() &  ( 1 << QEI_SW_PIN)))
	{
		uint16_t v = adc_poll_read(CONT_ADC_CHANNEL);
		if (v < 100)
		{
			ssd1306_invert(1);
			// Turn the LED on
			LPC_GPIO_PORT->CLR0 = (1 << LED_PIN);
			// Enable the DAC audio output for an audible alert
			dac_wavegen_run(WAVEGEN_DAC, app_cont_dac_output, sizeof(app_cont_dac_output)/2, 100);
		    GPIOSetBitValue(AUDIO_AMP_ENABLE_PORT, AUDIO_AMP_ENABLE_PIN, 1);
		}
		else
		{
			ssd1306_invert(0);
			// Turn the LED off
			LPC_GPIO_PORT->SET0 = (1 << LED_PIN);
			// Disable the DAC audio output
		    GPIOSetBitValue(AUDIO_AMP_ENABLE_PORT, AUDIO_AMP_ENABLE_PIN, 0);
		}
		ssd1306_refresh();
	}

	// Make sure to shut the DAC off before leaving
	dac_wavegen_stop(WAVEGEN_DAC);
}
