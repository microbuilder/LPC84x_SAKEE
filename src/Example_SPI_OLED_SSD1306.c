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

#include "config.h"
#include "adc_dma.h"
#include "qei.h"


#define MV_PER_LSB    (3300.0F / 0xFFF)

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
 P1.21  D4	  QEI0 switch/button
 P0.20  D3    SCT_IN0: QEI0 phA (internal pullup enabled)
 P0.21  D2    SCT_IN1: QEI0 phB (internal pullup enabled)

 DMA ADC
 LPC    ARD   Description
 -----  ---   -----------
 P0.14  A0    Analog Input (ADC2)

 LEDS
 LPC    ARD   Description
 -----  ---   -----------
 P0.0  	--	  Debug LED (GREEN)
*/

uint32_t button_pressed(void);

void led_on(void)
{
  LPC_GPIO_PORT->CLR0 = (1 << LED_PIN);
}

void led_off(void)
{
  LPC_GPIO_PORT->SET0 = (1 << LED_PIN);
}

static inline uint32_t button_read(void)
{
  // button is active LOW
  return ((~(LPC_GPIO_PORT->PIN[BUTTON_PIN/32])) & bit(BUTTON_PIN%32) );
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

	// Initialize Button
	GPIOSetDir(BUTTON_PIN/32, BUTTON_PIN%32, 0);

	// Initialize the SCT based quadrature decoder
	qei_init();

	// Initialize the DMA and systick based ADC sampler
	adc_dma_init();
	adc_dma_set_rate(100); // Set the ADC sample rate in microseconds

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

	while(1)
	{
		ssd1306_clear();
		ssd1306_set_text(8, 0, 1, "WAITING FOR", 2);
		ssd1306_set_text(8, 16, 1, "ADC TRIGGER:", 2);
		ssd1306_set_text(8, 32, 1, "2.95-3.05V", 2);
		ssd1306_set_text(8, 48, 1, "(3660..3785 LSB)", 1);
		ssd1306_refresh();

		if ( !adc_dma_busy() )
		{
		  // Start sampling with threshold detection (low, high, mode)
		  // interrupt mode: 0 = disabled, 1 = outside threshold, 2 = crossing threshold
		  // 2.95V..3.05V @ 3.3VREF = 3660..3785
		  adc_dma_start_with_threshold(3660, 3785, 2);
		  int16_t sample = adc_dma_get_threshold_sample();
		  if (sample < 0)
		  {
			  // ToDo: Error message
		  }
		  else
		  {
            gfx_graticule_cfg_t grcfg =
            {
              .w = 64,			// 64 pixels wide
              .h = 32,			// 32 pixels high
              .lines = GFX_GRATICULE_LINES_HOR | GFX_GRATICULE_LINES_VER,
              .line_spacing = 2,	// Divider lines are 1 dot every 2 pixels
              .block_spacing = 8	// Each block is 8x8 pixels
            };

            ssd1306_clear();

            // Render the title bars
            ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
            ssd1306_set_text(127 - 48, 0, 1, "WAVEFORM", 1);	// 48 pixels wide

            // Render the graticule and waveform
            gfx_graticule(0, 16, &grcfg, 1);
            // Make sure we have at least 32 samples before the trigger, or start at 0 if less
            uint16_t start = sample >= 32 ? sample - 32 : 0;
            gfx_waveform_64_32(0, 16, 1, adc_dma_get_buffer(), start, 1024, 4);

            // Labels
            uint16_t trig = adc_dma_get_buffer()[sample]>>4;
            float trig_mv = MV_PER_LSB * trig;
        	gfx_printdec(70, 16, (int32_t)trig_mv, 1, 1);
            ssd1306_set_text(70, 16, 1, "        mV", 1);
            ssd1306_set_text(70, 35, 1, "xx  ms/div", 1);
            ssd1306_set_text(70, 43, 1, "xx  mV/div", 1);

            // Refresh the display
            ssd1306_refresh();
		  }
		}

		// Wait for a button press to try again
		while (!button_pressed())
		{
			delay_ms(1);
		}
	}

	return 0;
}

/**
 * Check if a button has been pressed, including basic debouncing.
 *
 * Note: Press and hold will be reported as a single click, and
 *       release isn't reported in the code below. Only a single
 *       press event will register as a 1 at the appropriate bit.
 *
 * @return Bitmask of pressed buttons e.g If BUTTON_A is pressed
 * bit 31 will be set.
 */
uint32_t button_pressed(void)
{
  // must be exponent of 2
  enum { MAX_CHECKS = 2, SAMPLE_TIME = 5 };

  /* Array that maintains bounce status, which is sampled
   * at 10 ms/lsb. Debounced state is valid if all values
   * on a switch maintain the same state (bit set or clear)
   */
  static uint32_t lastReadTime = 0;
  static uint32_t states[MAX_CHECKS] = { 0 };
  static uint32_t index = 0;

  // Last debounce state, used to detect changes
  static uint32_t lastDebounced = 0;

  // Too soon, nothing to do
  if (millis() - lastReadTime < SAMPLE_TIME ) return 0;

  lastReadTime = millis();

  // Take current read and mask with BUTTONs
  // Note: Bitwise inverted since buttons are active (pressed) LOW
  uint32_t debounced = button_read();

  // Copy current state into array
  states[ (index & (MAX_CHECKS-1)) ] = debounced;
  index++;

  // Bitwise 'and' all the states in the array together to get the result
  // Pin must stay asserted at least MAX_CHECKS time to be recognized as valid
  for(int i=0; i<MAX_CHECKS; i++)
  {
    debounced &= states[i];
  }

  // 'result' = button changed and passes debounce checks, 0 = failure or not-asserted
  uint32_t result = (debounced ^ lastDebounced) & debounced;

  lastDebounced = debounced;

  return result;
}
