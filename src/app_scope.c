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

#define APP_SCOPE_WAVEFORM_RENDER_AS_BAR	(0)	// Set this to 1 to render waveform with solid bars from bottom to sample height

app_scope_rate_t _app_scope_rate;
uint16_t         _app_scope_thresh_l = (uint16_t)(1001/MV_PER_LSB); // Default lower threshold in lsb
uint16_t         _app_scope_thresh_h = (uint16_t)(1100/MV_PER_LSB); // Default upper threshold in lsb
uint8_t          _app_scope_coupling = 0;		// 0 = DC, 1 = AC (default = DC)
uint8_t          _app_scope_vdiv = 0;           // 0 = No input divider, 1 = Enable the 0.787X voltage divider

void app_scope_init(app_scope_rate_t rate)
{
	// Initialize the DMA and systick based ADC sampler
	adc_dma_init();
	_app_scope_rate = rate;
	adc_dma_set_rate(rate); // Set the ADC default sample rate in microseconds

	// Analog front end setup
	GPIOSetDir(AN_IN_VREF_3_3V_0_971V/32, AN_IN_VREF_3_3V_0_971V%32, 1); /* 3.3V or 0.971V VRef (240K + 100K divider) */
	GPIOSetDir(AN_IN_VDIV_0_787X/32, AN_IN_VDIV_0_787X%32, 1); 			 /* 0.787X voltage divider bypass */
	GPIOSetDir(AN_IN_220NF_BLOCKING/32, AN_IN_220NF_BLOCKING%32, 1); 	 /* 220nF inline AC/DC blocking cap bypass */

	// Toggle 0.971V VRef (3.3V by default)
	// Note: This MUST be 3.3V since the minimum VREFP on the LPC845 is 2.4V
	if (AN_IN_VREF_3_3V_0_971V/32)
	{
		LPC_GPIO_PORT->SET1 = (1 << (AN_IN_VREF_3_3V_0_971V%32));
	}
	else
	{
		LPC_GPIO_PORT->SET0 = (1 << (AN_IN_VREF_3_3V_0_971V%32));
	}

	// Toggle 0.787X (27K+100K) Voltage Divider (Full 3.3V input range by default)
	if (AN_IN_VDIV_0_787X/32)
	{
		if (_app_scope_vdiv)
		{
			// Enable the 0.787x voltage divider
			LPC_GPIO_PORT->CLR1 = (1 << (AN_IN_VDIV_0_787X%32));
		}
		else
		{
			// No voltage divider
			LPC_GPIO_PORT->SET1 = (1 << (AN_IN_VDIV_0_787X%32));
		}
	}
	else
	{
		if (_app_scope_vdiv)
		{
			// Enable the 0.787x voltage divider
			LPC_GPIO_PORT->CLR0 = (1 << (AN_IN_VDIV_0_787X%32));
		}
		else
		{
			// No voltage divider
			LPC_GPIO_PORT->SET0 = (1 << (AN_IN_VDIV_0_787X%32));
		}
	}

	// Toggle 220nF AC/DC blocking cap (DC coupling by default)
	if (AN_IN_220NF_BLOCKING/32)
	{
		if (_app_scope_coupling)
		{
			// AC coupling
			LPC_GPIO_PORT->CLR1 = (1 << (AN_IN_220NF_BLOCKING%32));
		}
		else
		{
			// DC coupling
			LPC_GPIO_PORT->SET1 = (1 << (AN_IN_220NF_BLOCKING%32));
		}
	}
	else
	{
		if (_app_scope_coupling)
		{
			// AC coupling
			LPC_GPIO_PORT->CLR0 = (1 << (AN_IN_220NF_BLOCKING%32));
		}
		else
		{
			// DC coupling
			LPC_GPIO_PORT->SET0 = (1 << (AN_IN_220NF_BLOCKING%32));
		}
	}

	ssd1306_clear();

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

	// Render AD/DC coupling indicator
	ssd1306_set_text(127-18, 8, 1, _app_scope_coupling ? "AC" : "DC", 1);

	// Display voltage divider warning if enabled
	if (_app_scope_vdiv)
	{
		ssd1306_set_text(70, 8, 1, "0.787x", 1);
	}

	// Render the graticule and waveform
	gfx_graticule(0, 16, &grcfg, 1);
	// Make sure we have at least 32 samples before the trigger, or start at 0 if less
	uint16_t start = sample >= 32 ? sample - 32 : 0;
	// ToDo: Check for overflow in adc_buffer
	gfx_waveform_64_32(0, 16, 1, adc_dma_get_buffer(), start, 2048, 4, APP_SCOPE_WAVEFORM_RENDER_AS_BAR);

	uint8_t meas_x = sample >= 32 ? 32 : sample;

	// Render the measurement point triangle
	ssd1306_set_pixel(meas_x, 13, 1);
	ssd1306_set_pixel(meas_x-1, 12, 1);
	ssd1306_set_pixel(meas_x, 12, 1);
	ssd1306_set_pixel(meas_x+1, 12, 1);
	ssd1306_set_pixel(meas_x-2, 11, 1);
	ssd1306_set_pixel(meas_x-1, 11, 1);
	ssd1306_set_pixel(meas_x, 11, 1);
	ssd1306_set_pixel(meas_x+1, 11, 1);
	ssd1306_set_pixel(meas_x+2, 11, 1);

	// Labels
	uint16_t trig = adc_dma_get_buffer()[sample]>>4;
	float trig_mv = MV_PER_LSB * trig;
	uint32_t us_per_div = adc_dma_get_rate() * 8;
	gfx_printdec(70, 16, (int32_t)trig_mv, 1, 1);
	ssd1306_set_text(70, 16, 1, "        mV", 1);
	ssd1306_set_text(70, 24, 1, "        us", 1);
	if (offset_us >= 0)
	{
		ssd1306_set_text(70, 24, 1, "+", 1);
		gfx_printdec(76, 24, offset_us, 1, 1);
	}
	else
	{
		gfx_printdec(70, 24, offset_us, 1, 1);
	}
	ssd1306_set_text(70, 35, 1, "    us/div", 1);
	gfx_printdec(70, 35, (int32_t)us_per_div, 1, 1);
	ssd1306_set_text(70, 43, 1, "    mV/div", 1);
	gfx_printdec(70, 43, (int32_t)(3300/4), 1, 1);

	ssd1306_set_text(16, 55, 1, "CLICK FOR MAIN MENU", 1);

	// Refresh the display
	ssd1306_refresh();
}

void app_scope_arm_trigger(void)
{
	static int32_t last_position_qei = 0;
	int16_t sample = 0;

	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-48, 0, 1, "WAVEFORM", 1);	// 48 pixels wide

    // Render the bottom button options
	ssd1306_fill_rect(0, 55, 128, 8, 1);
    //ssd1306_set_text(96, 56, 0, "STOP", 1);

	ssd1306_set_text(6, 16, 1,  "WAITING FOR", 2);
	ssd1306_set_text(6, 32, 1, "ADC TRIGGER", 2);

    ssd1306_refresh();

	// Reset the QEI encoder position counter
	qei_reset_step();

	if ( !adc_dma_busy() )
	{
	  // Start sampling with threshold detection (low, high, mode)
	  // interrupt mode: 0 = disabled, 1 = outside threshold, 2 = crossing threshold
	  adc_dma_start_with_threshold(_app_scope_thresh_l, _app_scope_thresh_h, 2);
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

	// Wait for a button press to escape waveform analysis
	while (!button_pressed())
	{
		// Check for a scroll request on the QEI
		// QEI scroll = adjust waveform offset
		int32_t abs = qei_abs_step();
		// Don't allow scrolling outside the leading edge of the waveform
		if (sample+abs <= 0)
		{
			abs = sample * -1;
			qei_reset_step_val(abs);
		}
		// Stay within sample+1K sample upper limit
		if (sample+abs > sample+992)
		{
			abs = 992;
			qei_reset_step_val(abs);
		}
		// Adjust waveform offset on qei scroll
		if (abs != last_position_qei)
		{
			app_scope_render_waveform(sample+abs, (int32_t)abs*adc_dma_get_rate());
			last_position_qei = abs;
		}

		delay_ms(1);
	}
}

void app_scope_render_threshold(uint16_t low, uint16_t high)
{
	ssd1306_fill_rect(0, 24, 128, 31, 0);
    float lower_v = MV_PER_LSB * (float)low;
    float upper_v = MV_PER_LSB * (float)high;
    ssd1306_set_text(0, 28, 1,  "TRIG L:", 1);
    ssd1306_set_text(0, 44, 1,  "TRIG H:", 1);
	gfx_printdec(40, 24, (int32_t)lower_v, 2, 1);
	gfx_printdec(40, 40, (int32_t)upper_v, 2, 1);
    ssd1306_set_text(40, 24, 1, "     mV", 2);
    ssd1306_set_text(40, 40, 1, "     mV", 2);
}

void app_scope_render_rate(app_scope_rate_t rate)
{
	ssd1306_fill_rect(0, 12, 128, 16, 0);
    ssd1306_set_text(0, 12, 1, "RATE:", 1);
    switch(rate)
    {
    case APP_SCOPE_RATE_500_KHZ:
        ssd1306_set_text(40, 8, 1, "500  kHz", 2);
    	break;
    case APP_SCOPE_RATE_250_KHZ:
        ssd1306_set_text(40, 8, 1, "250  kHz", 2);
    	break;
    case APP_SCOPE_RATE_100_KHZ:
        ssd1306_set_text(40, 8, 1, "100  kHz", 2);
    	break;
    case APP_SCOPE_RATE_50_KHZ:
        ssd1306_set_text(40, 8, 1, "50   kHz", 2);
    	break;
    case APP_SCOPE_RATE_25_KHZ:
        ssd1306_set_text(40, 8, 1, "25   kHz", 2);
    	break;
    case APP_SCOPE_RATE_10_KHZ:
        ssd1306_set_text(40, 8, 1, "10   kHz", 2);
    	break;
    case APP_SCOPE_RATE_5_KHZ:
        ssd1306_set_text(40, 8, 1, "5    kHz", 2);
    	break;
    case APP_SCOPE_RATE_2_5_KHZ:
        ssd1306_set_text(40, 8, 1, "2.5  kHz", 2);
    	break;
    case APP_SCOPE_RATE_1_KHZ:
        ssd1306_set_text(40, 8, 1, "1    kHz", 2);
    	break;
    case APP_SCOPE_RATE_500_HZ:
        ssd1306_set_text(40, 8, 1, "500  Hz", 2);
    	break;
    case APP_SCOPE_RATE_250_HZ:
        ssd1306_set_text(40, 8, 1, "250  Hz", 2);
    	break;
    case APP_SCOPE_RATE_100_HZ:
        ssd1306_set_text(40, 8, 1, "100  Hz", 2);
    	break;
    case APP_SCOPE_RATE_60_HZ:
        ssd1306_set_text(40, 8, 1, "60   Hz", 2);
    	break;
    case APP_SCOPE_RATE_50_HZ:
        ssd1306_set_text(40, 8, 1, "50   Hz", 2);
    	break;
    case APP_SCOPE_RATE_25_HZ:
        ssd1306_set_text(40, 8, 1, "25   Hz", 2);
    	break;
    case APP_SCOPE_RATE_10_HZ:
        ssd1306_set_text(40, 8, 1, "10   Hz", 2);
    	break;
    }
}

void app_scope_run(void)
{
	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-48, 0, 1, "WAVEFORM", 1);	// 48 pixels wide

    // Render the bottom button options
	//ssd1306_fill_rect(0, 55, 128, 8, 1);
    //ssd1306_set_text(8, 56, 0, "HOME", 1);
    //ssd1306_set_text(36, 56, 0, "TIME", 1);
    //ssd1306_set_text(64, 56, 0, "TRIG", 1);
    //ssd1306_set_text(96, 56, 0, "RUN", 1);

    // Display the current sample rate
    app_scope_render_rate(_app_scope_rate);

    // Display the current lower/upper thresholds
    app_scope_render_threshold(_app_scope_thresh_l, _app_scope_thresh_h);

    // Help message
	ssd1306_set_text(0, 55, 1, "ROTATE=TRIGGER SEL=START", 1);

    ssd1306_refresh();

	// Reset the QEI encoder position counter
	int32_t last_position_qei = 0;
	qei_reset_step();

    // Wait for the button to arm the trigger
	while (!(button_pressed() &  ( 1 << QEI_SW_PIN)))
    {
		// Check for a scroll request on the QEI
        // Adjust the trigger level by default if any rotation occurs
		int32_t abs = qei_abs_step();
		// Adjust waveform offset on qei scroll
		if (abs != last_position_qei)
		{
			float offset = ((abs - last_position_qei) * 50.0F)/MV_PER_LSB;
			// Stay above 0V
			if ((int16_t)_app_scope_thresh_l + (int16_t)offset < 0)
			{
				_app_scope_thresh_l = 0;
				_app_scope_thresh_h = (uint16_t)(100.0F/MV_PER_LSB);
			}
			// Stay below VCC
			else if ((int16_t)_app_scope_thresh_l + (int16_t)offset > 4095 - (int16_t)(100.0F/MV_PER_LSB))
			{
				_app_scope_thresh_l = 4095 - (uint16_t)(100.0F/MV_PER_LSB);
				_app_scope_thresh_h = 4095;
			}
			else
			{
				_app_scope_thresh_l += (int16_t)offset;
				_app_scope_thresh_h = _app_scope_thresh_l + (uint16_t)(100.0F/MV_PER_LSB);
			}

			// Update the display
			app_scope_render_threshold(_app_scope_thresh_l, _app_scope_thresh_h);
			ssd1306_refresh();

			last_position_qei = abs;
		}

    	delay_ms(1);
    }

    // Always start trigger for now
    app_scope_arm_trigger();
}
