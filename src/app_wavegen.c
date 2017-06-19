/*
===============================================================================
 Name        : app_wavegen.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Basic DAC based signal generator
===============================================================================
 */

#include "LPC8xx.h"
#include "dac.h"

#include "config.h"
#include "delay.h"
#include "button.h"
#include "qei.h"
#include "gfx.h"
#include "app_wavegen.h"
#include "dac_wavegen.h"

typedef enum
{
	APP_WAVEGEN_WAVE_SINE = 0,
	APP_WAVEGEN_WAVE_TRIANGLE = 1,
	APP_WAVEGEN_WAVE_EXPDECAY = 2,
	APP_WAVEGEN_WAVE_LAST
} app_wavegen_wave_t;

static app_wavegen_wave_t _app_wavegen_curwave = APP_WAVEGEN_WAVE_SINE;

static const uint16_t app_wavegen_sine_wave[64] = {
	0x200, 0x232, 0x263, 0x294, 0x2c3, 0x2f1, 0x31c, 0x344,
	0x369, 0x38b, 0x3a9, 0x3c3, 0x3d8, 0x3e9, 0x3f5, 0x3fd,
	0x3ff, 0x3fd, 0x3f5, 0x3e9, 0x3d8, 0x3c3, 0x3a9, 0x38b,
	0x369, 0x344, 0x31c, 0x2f1, 0x2c3, 0x294, 0x263, 0x232,
	0x200, 0x1cd, 0x19c, 0x16b, 0x13c, 0x10e, 0x0e3, 0x0bb,
	0x096, 0x074, 0x056, 0x03c, 0x027, 0x016, 0x00a, 0x002,
	0x000, 0x002, 0x00a, 0x016, 0x027, 0x03c, 0x056, 0x074,
	0x096, 0x0bb, 0x0e3, 0x10e, 0x13c, 0x16b, 0x19c, 0x1cd
};

static const uint16_t app_wavegen_triangle_wave[64] = {
	0x020, 0x040, 0x060, 0x080, 0x0a0, 0x0c0, 0x0e0, 0x100,
	0x120, 0x140, 0x160, 0x180, 0x1a0, 0x1c0, 0x1e0, 0x200,
	0x21f, 0x23f, 0x25f, 0x27f, 0x29f, 0x2bf, 0x2df, 0x2ff,
	0x31f, 0x33f, 0x35f, 0x37f, 0x39f, 0x3bf, 0x3df, 0x3ff,
	0x3df, 0x3bf, 0x39f, 0x37f, 0x35f, 0x33f, 0x31f, 0x2ff,
	0x2df, 0x2bf, 0x29f, 0x27f, 0x25f, 0x23f, 0x21f, 0x200,
	0x1e0, 0x1c0, 0x1a0, 0x180, 0x160, 0x140, 0x120, 0x100,
	0x0e0, 0x0c0, 0x0a0, 0x080, 0x060, 0x040, 0x020, 0x000
};

static const uint16_t app_wavegen_expdecay_wave[64] = {
	0x3ff, 0x3c1, 0x387, 0x350, 0x31d, 0x2ec, 0x2bf, 0x294,
	0x26c, 0x247, 0x224, 0x202, 0x1e3, 0x1c6, 0x1aa, 0x191,
	0x178, 0x162, 0x14c, 0x138, 0x125, 0x113, 0x103, 0x0f3,
	0x0e4, 0x0d6, 0x0c9, 0x0bd, 0x0b2, 0x0a7, 0x09d, 0x093,
	0x08a, 0x082, 0x07a, 0x073, 0x06c, 0x065, 0x05f, 0x059,
	0x054, 0x04f, 0x04a, 0x046, 0x041, 0x03d, 0x03a, 0x036,
	0x033, 0x030, 0x02d, 0x02a, 0x028, 0x025, 0x023, 0x021,
	0x01f, 0x01d, 0x01b, 0x01a, 0x018, 0x017, 0x015, 0x014
};

void app_wavegen_init(void)
{
  ssd1306_clear();
  ssd1306_refresh();
  dac_wavegen_init(WAVEGEN_DAC);
}

void app_wavegen_render_setup()
{
  gfx_graticule_cfg_t grcfg =
  {
      .w = 64,			// 64 pixels wide
      .h = 32,			// 32 pixels high
      .lines = GFX_GRATICULE_LINES_TOP | GFX_GRATICULE_LINES_BOT,
      .line_spacing = 2,	// Divider lines are 1 dot every 2 pixels
      .block_spacing = 8	// Each block is 8x8 pixels
  };

  ssd1306_init();
  ssd1306_clear();

  // Render the title bars
  ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
  ssd1306_set_text(127 - 60, 0, 1, "DAC WAVEGEN", 1);
  ssd1306_set_text(16, 55, 1, "CLICK FOR MAIN MENU", 1);

  // Render the graticule and waveform
  gfx_graticule(0, 16, &grcfg, 1);

  // Render a waveform
  switch(_app_wavegen_curwave)
  {
  	  case APP_WAVEGEN_WAVE_LAST:
	  case APP_WAVEGEN_WAVE_SINE:
		  gfx_waveform_64_32_10bit(0, 16, 1, app_wavegen_sine_wave, 0, sizeof(app_wavegen_sine_wave) / 2, 0, 0);
		  dac_wavegen_run(WAVEGEN_DAC, app_wavegen_sine_wave, sizeof(app_wavegen_sine_wave)/2, 200);
		  ssd1306_set_text(70, 16, 1, "WFRM SINE", 1);
		  break;
	  case APP_WAVEGEN_WAVE_TRIANGLE:
		  gfx_waveform_64_32_10bit(0, 16, 1, app_wavegen_triangle_wave, 0, sizeof(app_wavegen_triangle_wave) / 2, 0, 0);
		  dac_wavegen_run(WAVEGEN_DAC, app_wavegen_triangle_wave, sizeof(app_wavegen_triangle_wave)/2, 200);
		  ssd1306_set_text(70, 16, 1, "WFRM TRIA", 1);
		  break;
	  case APP_WAVEGEN_WAVE_EXPDECAY:
		  gfx_waveform_64_32_10bit(0, 16, 1, app_wavegen_expdecay_wave, 0, sizeof(app_wavegen_expdecay_wave) / 2, 0, 0);
		  dac_wavegen_run(WAVEGEN_DAC, app_wavegen_expdecay_wave, sizeof(app_wavegen_expdecay_wave)/2, 200);
		  ssd1306_set_text(70, 16, 1, "WFRM EXPO", 1);
		  break;
  }

  // Render some labels
  ssd1306_set_text(70, 24, 1, "FREQ 200 Hz", 1);
  ssd1306_set_text(70, 32, 1, "AMPL 3.3 V", 1);

  ssd1306_refresh();
}

void app_wavegen_run(void)
{
	app_wavegen_render_setup();

	// Reset the QEI encoder position counter
	int32_t last_position_qei = 0;
	qei_reset_step();

	while (!button_pressed())
	{
		/*
	    // Enable speaker or DAC GPIO
		GPIOSetDir(DAC1EN_PIN/32, DAC1EN_PIN%32, 1);
		LPC_GPIO_PORT->CLR0 = (1 << DAC1EN_PIN);
		*/

		// Check for a scroll request on the QEI
        // Adjust the trigger level by default if any rotation occurs
		int32_t abs = qei_abs_step();
		// Adjust waveform offset on qei scroll
		if (abs != last_position_qei)
		{
			int16_t pos = (int16_t)_app_wavegen_curwave;
			pos += (abs - last_position_qei);
			if (pos > APP_WAVEGEN_WAVE_EXPDECAY)
			{
				pos = APP_WAVEGEN_WAVE_SINE;
			}
			if (pos < 0)
			{
				pos = APP_WAVEGEN_WAVE_EXPDECAY;
			}
			_app_wavegen_curwave = (app_wavegen_wave_t)pos;
			app_wavegen_render_setup();
			ssd1306_refresh();
			last_position_qei = abs;
		}
	}

	dac_wavegen_stop(WAVEGEN_DAC);
}
