/*
===============================================================================
 Name        : app_scope.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Basic oscilloscope app
===============================================================================
 */

#include "LPC8xx.h"
#include "gpio.h"

#include "config.h"
#include "delay.h"
#include "qei.h"
#include "adc_dma.h"
#include "button.h"
#include "gfx.h"
#include "app_scope.h"

void app_scope_init(void)
{
	// Initialize the DMA and systick based ADC sampler
	adc_dma_init();
	adc_dma_set_rate(100); // Set the ADC sample rate in microseconds


	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-48, 0, 1, "WAVEFORM", 1);	// 48 pixels wide

    // Render the bottom button options
	ssd1306_fill_rect(0, 55, 128, 8, 1);
    ssd1306_set_text(8, 56, 0, "HOME", 1);
    ssd1306_set_text(36, 56, 0, "TIME", 1);
    ssd1306_set_text(64, 56, 0, "TRIG", 1);
    ssd1306_set_text(96, 56, 0, "STOP", 1);

	// Refresh the display
	ssd1306_refresh();
}

void app_scope_render_waveform(int16_t sample, int32_t offset_us)
{
	gfx_graticule_cfg_t grcfg =
	{
	  .w = 64,			// 64 pixels wide
	  .h = 32,			// 32 pixels high
	  .lines = GFX_GRATICULE_LINES_NONE, // GFX_GRATICULE_LINES_HOR | GFX_GRATICULE_LINES_VER,
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
	// ToDo: Check for overflow in adc_buffer
	gfx_waveform_64_32(0, 16, 1, adc_dma_get_buffer(), start, 1024, 4);

	// Render the measurement point triangle
	ssd1306_set_pixel(32, 13, 1);
	ssd1306_set_pixel(31, 12, 1);
	ssd1306_set_pixel(32, 12, 1);
	ssd1306_set_pixel(33, 12, 1);
	ssd1306_set_pixel(30, 11, 1);
	ssd1306_set_pixel(31, 11, 1);
	ssd1306_set_pixel(32, 11, 1);
	ssd1306_set_pixel(33, 11, 1);
	ssd1306_set_pixel(34, 11, 1);

	// Labels
	uint16_t trig = adc_dma_get_buffer()[sample]>>4;
	float trig_mv = MV_PER_LSB * trig;
	uint32_t us_per_div = adc_dma_get_rate() * 8;
	gfx_printdec(70, 16, (int32_t)trig_mv, 1, 1);
	ssd1306_set_text(70, 16, 1, "        mV", 1);
	ssd1306_set_text(70, 27, 1, "        us", 1);
	gfx_printdec(70, 27, offset_us, 1, 1);
	ssd1306_set_text(70, 35, 1, "    us/div", 1);
	gfx_printdec(70, 35, (int32_t)us_per_div, 1, 1);
	ssd1306_set_text(70, 43, 1, "    mV/div", 1);
	gfx_printdec(70, 43, (int32_t)(3300/4), 1, 1);

	// Refresh the display
	ssd1306_refresh();
}

void app_scope_run(void)
{
	static int32_t last_position_qei = 0;
	int16_t sample = 0;

	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-48, 0, 1, "WAVEFORM", 1);	// 48 pixels wide

	ssd1306_set_text(0, 8, 1, "WAITING FOR", 2);
	ssd1306_set_text(0, 24, 1, "ADC TRIGGER:", 2);
	ssd1306_set_text(0, 40, 1, "2.95-3.05V", 2);
	ssd1306_set_text(0, 56, 1, "(3660..3785 LSB)", 1);
	ssd1306_refresh();

	// Reset the QEI encoder position counter
	qei_reset_step();

	if ( !adc_dma_busy() )
	{
	  // Start sampling with threshold detection (low, high, mode)
	  // interrupt mode: 0 = disabled, 1 = outside threshold, 2 = crossing threshold
	  // 2.95V..3.05V @ 3.3VREF = 3660..3785
	  adc_dma_start_with_threshold(3660, 3785, 2);
	  sample = adc_dma_get_threshold_sample();
	  if (sample < 0)
	  {
		  // ToDo: Error message
	  }
	  else
	  {
		  app_scope_render_waveform(sample, 0);
	  }
	}

	// Wait for a button press to try again
	while (!button_pressed())
	{
		// Check for a scroll request on the QEI
		int32_t abs = qei_abs_step();
		if (abs != last_position_qei)
		{
			// Adjust waveform offset
			app_scope_render_waveform(sample+abs, (int32_t)abs*adc_dma_get_rate());
			last_position_qei = abs;
		}

		delay_ms(1);
	}
}
