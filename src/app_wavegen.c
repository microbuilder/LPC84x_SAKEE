/*
===============================================================================
 Name        : app_wavegen.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Basic DAC based signal generator
===============================================================================
 */

#include <string.h>

#include "LPC8xx.h"
#include "dac.h"
#include "gpio.h"

#include "config.h"
#include "delay.h"
#include "button.h"
#include "qei.h"
#include "gfx.h"
#include "app_wavegen.h"
#include "dac_wavegen.h"

// Note that the DAC is limited to approx. VDD-1.5V, so
// on a 3.3V system DAC input values should be restricted
// to a 0-1.8V range (0..558) since:
// 3300mV / 1024 = 3.22265625mV per lsb
#define APP_WAVEGEN_MAX_DAC_INPUT	(558)

// Beyond about 800Hz the DAC IRQ fires too rapidly (<200 ticks)
// and overwhelms the rest of the system at the default clock
// speed. The clock will need to be raised to go beyond 800Hz.
#define APP_WAVEGEN_HZ_MIN          (100)
#define APP_WAVEGEN_HZ_MAX          (800)

typedef enum
{
	APP_WAVEGEN_WAVE_SINE = 0,
	APP_WAVEGEN_WAVE_TRIANGLE = 1,
	APP_WAVEGEN_WAVE_EXPDECAY = 2,
	APP_WAVEGEN_WAVE_USER = 3,
	APP_WAVEGEN_WAVE_LAST
} app_wavegen_wave_t;

static app_wavegen_wave_t _app_wavegen_curwave = APP_WAVEGEN_WAVE_SINE;
static uint16_t _app_wavegen_frequency_hz = 200;
static uint8_t _app_wavegen_output_spkr = 0;

static const uint16_t app_wavegen_sine_wave[64] = {
	279, 306, 333, 360, 386, 411, 434, 456,
	476, 495, 511, 525, 537, 546, 553, 557,
	558, 557, 553, 546, 537, 525, 511, 495,
	476, 456, 434, 411, 386, 360, 333, 306,
	279, 252, 225, 198, 172, 147, 124, 102,
	 82,  63,  47,  33,  21,  12,   5,   1,
	   0,   1,   5,  12,  21,  33,  47,  63,
	  82, 102, 124, 147, 172, 198, 225, 252
};

static const uint16_t app_wavegen_triangle_wave[64] = {
	  17,  35,  52,  70,  87, 105, 122, 140,
	 157, 174, 192, 209, 227, 244, 262, 279,
	 296, 314, 331, 349, 366, 384, 401, 419,
	 436, 453, 471, 488, 506, 523, 541, 558,
	 541, 523, 506, 488, 471, 453, 436, 419,
	 401, 384, 366, 349, 331, 314, 296, 279,
	 262, 244, 227, 209, 192, 174, 157, 140,
	 122, 105,  87,  70,  52,  35,  17,   0
};

static const uint16_t app_wavegen_expdecay_wave[64] = {
	   0,  34,  66,  95, 123, 150, 174, 198,
	 220, 240, 259, 277, 294, 310, 325, 339,
	 353, 365, 377, 388, 398, 408, 417, 425,
	 433, 441, 448, 455, 461, 467, 472, 478,
	 482, 487, 491, 495, 499, 503, 506, 509,
	 512, 515, 518, 520, 522, 524, 527, 528,
	 530, 532, 533, 535, 536, 538, 539, 540,
	 541, 542, 543, 544, 545, 546, 546, 547
};

static uint16_t app_wavegen_user_wave[64] = {
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000
};

void app_wavegen_init(void)
{
  ssd1306_clear();
  ssd1306_refresh();
  dac_wavegen_init(WAVEGEN_DAC);
}

void app_wavegen_uart_help_msg(void)
{
	// Render a hint out to the UART port for new USER waveform
	printf("WAVEGEN USER WAVEFORM UPLOAD:\n\r");
	printf("- To update the USER waveform send 64 '\\r' separated 10-bit\n\r" \
		   "  values in decimal format (0..1023\\r).\n\r" \
		   "- An 'OK[<samplenum>=<value>]\\n\\r' string will be sent after each\n\r" \
		   "  valid sample, and at the end of the sequence a 'DONE\\n\\r' message\n\r" \
		   "  will be sent, and the user waveform will be persisted to EEPROM.\n\r" \
		   "- 'ERROR: *\\n\\r' at any time indicates a failure.\n\r");
}

void app_wavegen_uart_read_waveform(void)
{
	char ch;
	int i;
	uint16_t count;

	ch = i = count = 0;

	app_wavegen_uart_help_msg();

	while (count < 64)
	{
		// Display the input prompt
		printf("> ");

		// Wait for some numeric input or a carriage-return character
		// This can be adjusted to wait for newline (\n), but minicom
		// was used for development on an OS X system with the following
		// parameters, which will default to sending '\r' for return:
		//   $ minicom -D /dev/tty.usbmodem1422 -b 9600
		while ((ch = getchar()) != '\r')
		{
			// Display help message on '?'
			if (ch == '?')
			{
				app_wavegen_uart_help_msg();
				return;
			}
			// Parse integers ... this will also eat any non numeric characters
			if (ch <= '9' && ch >= '0')
			{
				i *= 10;
				i += ch - '0';
			}
		}

		// Ensure a 10-bit range
		if (i > 1023)
		{
			printf("ERROR: Numeric overflow\n\r");
			app_wavegen_uart_help_msg();
			return;
		}
		// VDD-1.5V restriction warning
		if (i > APP_WAVEGEN_MAX_DAC_INPUT)
		{
			printf("WARNING: 1.8V limit exceeded, trimming value to %d", APP_WAVEGEN_MAX_DAC_INPUT);
			i = APP_WAVEGEN_MAX_DAC_INPUT;
		}

		// Add the new value to the user waveform
		app_wavegen_user_wave[count] = (uint16_t)i;
		printf("OK[%d=%d]\n\r", count+1, i);

		// Reset the integer placeholder
		i = 0;

		if (count == 63)
		{
			// ToDo: Persist user waveform to EEPROM!
			printf("DONE\n\r");
		}

		// Increment the counter
		count++;
	}
}

void app_wavegen_render_upload(void)
{
	ssd1306_clear();

	// Render the title bars
	ssd1306_set_text(0, 0, 1, "LPC SAKEE", 1);
	ssd1306_set_text(127 - 60, 0, 1, "DAC WAVEGEN", 1);

	// Help Text
    ssd1306_set_text(0, 12, 1, "OPEN USB SERIAL AT 9600", 1);
    ssd1306_set_text(0, 20, 1, "8N1 AND SEND 64 10-BIT", 1);
    ssd1306_set_text(0, 28, 1, "SAMPLES USING DECIMAL", 1);
    ssd1306_set_text(0, 36, 1, "VALUES (0..1023).", 1);

	// Render the bottom button options
    ssd1306_set_text(16, 55, 1, "CLICK FOR MAIN MENU", 1);

	// Display the screen contents
	ssd1306_refresh();
}

void app_wavegen_render_setup(void)
{
  gfx_graticule_cfg_t grcfg =
  {
      .w = 64,			// 64 pixels wide
      .h = 32,			// 32 pixels high
      .lines = GFX_GRATICULE_LINES_TOP | GFX_GRATICULE_LINES_BOT,
      .line_spacing = 2,	// Divider lines are 1 dot every 2 pixels
      .block_spacing = 8	// Each block is 8x8 pixels
  };

  //ssd1306_init();
  ssd1306_clear();

  // Render the title bars
  ssd1306_set_text(0, 0, 1, "LPC SAKEE", 1);
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
		  dac_wavegen_run(WAVEGEN_DAC, app_wavegen_sine_wave, sizeof(app_wavegen_sine_wave)/2, _app_wavegen_frequency_hz);
		  ssd1306_set_text(70, 16, 1, "WFRM SINE", 1);
		  break;
	  case APP_WAVEGEN_WAVE_TRIANGLE:
		  gfx_waveform_64_32_10bit(0, 16, 1, app_wavegen_triangle_wave, 0, sizeof(app_wavegen_triangle_wave) / 2, 0, 0);
		  dac_wavegen_run(WAVEGEN_DAC, app_wavegen_triangle_wave, sizeof(app_wavegen_triangle_wave)/2, _app_wavegen_frequency_hz);
		  ssd1306_set_text(70, 16, 1, "WFRM TRIA", 1);
		  break;
	  case APP_WAVEGEN_WAVE_EXPDECAY:
		  gfx_waveform_64_32_10bit(0, 16, 1, app_wavegen_expdecay_wave, 0, sizeof(app_wavegen_expdecay_wave) / 2, 0, 0);
		  dac_wavegen_run(WAVEGEN_DAC, app_wavegen_expdecay_wave, sizeof(app_wavegen_expdecay_wave)/2, _app_wavegen_frequency_hz);
		  ssd1306_set_text(70, 16, 1, "WFRM EXPO", 1);
		  break;
	  case APP_WAVEGEN_WAVE_USER:
		  gfx_waveform_64_32_10bit(0, 16, 1, app_wavegen_user_wave, 0, sizeof(app_wavegen_user_wave) / 2, 0, 0);
		  dac_wavegen_run(WAVEGEN_DAC, app_wavegen_user_wave, sizeof(app_wavegen_user_wave)/2, _app_wavegen_frequency_hz);
		  ssd1306_set_text(70, 16, 1, "WFRM USER", 1);
		  break;
  }

  // Render some labels
  ssd1306_set_text(70, 24, 1, "FREQ", 1);
  gfx_printdec(94, 24, _app_wavegen_frequency_hz, 1, 1);
  ssd1306_set_text(94+(gfx_num_digits(_app_wavegen_frequency_hz)*6), 24, 1, "Hz", 1);
  ssd1306_set_text(70, 32, 1, "AMPL 1.8 V", 1);
  ssd1306_set_text(70, 40, 1, _app_wavegen_output_spkr ? "SPKR OUT" : "DAC1 OUT", 1);

  ssd1306_refresh();
}

void app_wavegen_config_set_hz(void)
{
	// Reset the QEI encoder position counter
	int32_t last_position_qei = 0;
	qei_reset_step();

	ssd1306_clear();

	// Render the title bars
	ssd1306_set_text(0, 0, 1, "LPC SAKEE", 1);
	ssd1306_set_text(127 - 60, 0, 1, "DAC WAVEGEN", 1);
	ssd1306_set_text(15, 55, 1, "SELECT TO CONTINUE", 1);

	ssd1306_set_text(0, 12, 1, "SET OUTPUT FREQUENCY (Hz)", 1);
	gfx_printdec(40, 24, (int32_t)_app_wavegen_frequency_hz, 2, 1);
    ssd1306_set_text(40, 24, 1, "     Hz", 2);

	ssd1306_refresh();

    // Wait for the button to execute the selected sub-app
	while (!(button_pressed() &  ( 1 << QEI_SW_PIN)))
    {
		// Check for a scroll request on the QEI
		int32_t abs = qei_abs_step();
		if (abs != last_position_qei)
		{
			_app_wavegen_frequency_hz += qei_offset_step();
			if (_app_wavegen_frequency_hz < APP_WAVEGEN_HZ_MIN)
			{
				// Roll under to the top value
				_app_wavegen_frequency_hz = APP_WAVEGEN_HZ_MAX;
			}
			if (_app_wavegen_frequency_hz > APP_WAVEGEN_HZ_MAX)
			{
				// Roll over to the low value
				_app_wavegen_frequency_hz = APP_WAVEGEN_HZ_MIN;
			}
			ssd1306_fill_rect(0, 24, 128, 31, 0);
			gfx_printdec(40, 24, (int32_t)_app_wavegen_frequency_hz, 2, 1);
		    ssd1306_set_text(40, 24, 1, "     Hz", 2);
			ssd1306_refresh();
			// Track the position
			last_position_qei = abs;
		}
    }

	// Wait for the button to release
	while ((button_pressed() &  ( 1 << QEI_SW_PIN)))
	{
		delay_ms(10);
	}
}

int32_t app_wavegen_config_screen(void)
{
	enum
	{
		APP_WAVEGEN_CONFIG_DACOUT = 0,
		APP_WAVEGEN_CONFIG_SPKROUT,
		APP_WAVEGEN_CONFIG_USER,
		APP_WAVEGEN_CONFIG_CANCEL,
		APP_WAVEGEN_CONFIG_LAST,
	};

	int32_t menu_selected = APP_WAVEGEN_CONFIG_DACOUT;

	// Reset the QEI encoder position counter
	int32_t last_position_qei = 0;
	qei_reset_step();

	ssd1306_clear();

	// Render the title bars
	ssd1306_set_text(0, 0, 1, "LPC SAKEE", 1);
	ssd1306_set_text(127 - 60, 0, 1, "DAC WAVEGEN", 1);
	ssd1306_set_text(15, 55, 1, "SELECT TO CONTINUE", 1);

	// Render the config menu options
    ssd1306_set_text(10, 12, 1, "START W/DAC0 OUT", 1);
    ssd1306_set_text(10, 20, 1, "START W/SPEAKER OUT", 1);
    ssd1306_set_text(10, 28, 1, "UPLOAD USER WAVEFORM", 1);
    ssd1306_set_text(10, 36, 1, "CANCEL", 1);

    // Draw the initial selection indicator
    uint8_t y = ((menu_selected + 2) * 8) -2;
    ssd1306_set_pixel(6, y, 1);
    ssd1306_set_pixel(5, y-1, 1);
    ssd1306_set_pixel(5, y, 1);
    ssd1306_set_pixel(5, y+1, 1);
    ssd1306_set_pixel(4, y-2, 1);
    ssd1306_set_pixel(4, y-1, 1);
    ssd1306_set_pixel(4, y, 1);
    ssd1306_set_pixel(4, y+1, 1);
    ssd1306_set_pixel(4, y+2, 1);

	ssd1306_refresh();

    // Wait for the button to execute the selected sub-app
	while (!(button_pressed() &  ( 1 << QEI_SW_PIN)))
    {
		// Check for a scroll request on the QEI
		int32_t abs = qei_abs_step();
		if (abs != last_position_qei)
		{
			menu_selected += qei_offset_step();

			if (menu_selected < 0)
			{
				menu_selected = APP_WAVEGEN_CONFIG_LAST - 1;
			}
			if (menu_selected >= APP_WAVEGEN_CONFIG_LAST)
			{
				// Roll back to the first menu item
				menu_selected = 0;
			}

			// Track the position
			last_position_qei = abs;

			// Update the display
			ssd1306_fill_rect(4, 12, 3, 52, 0);
		    y = ((menu_selected + 2) * 8) -2;
		    ssd1306_set_pixel(6, y, 1);
		    ssd1306_set_pixel(5, y-1, 1);
		    ssd1306_set_pixel(5, y, 1);
		    ssd1306_set_pixel(5, y+1, 1);
		    ssd1306_set_pixel(4, y-2, 1);
		    ssd1306_set_pixel(4, y-1, 1);
		    ssd1306_set_pixel(4, y, 1);
		    ssd1306_set_pixel(4, y+1, 1);
		    ssd1306_set_pixel(4, y+2, 1);
			ssd1306_refresh();
		}

    	delay_ms(1);
    }

	switch (menu_selected)
	{
	case APP_WAVEGEN_CONFIG_DACOUT:
		// Start with DAC0 output
		_app_wavegen_output_spkr = 0;
		break;
	case APP_WAVEGEN_CONFIG_SPKROUT:
		// Start with SPKR output
		_app_wavegen_output_spkr = 1;
		break;
	case APP_WAVEGEN_CONFIG_USER:
		app_wavegen_render_upload();
		app_wavegen_uart_read_waveform();
		break;
	case APP_WAVEGEN_CONFIG_CANCEL:
	default:
		return -1;
	}

	// Update frequency on dedicated config page
	app_wavegen_config_set_hz();

	return 0;
}

void app_wavegen_run(void)
{
	// First run the config screen
	int32_t res = app_wavegen_config_screen();
	if (res == -1)
	{
		// User requests CANCEL/EXIT
		// To back to the main menu
		return;
	}

	// Then go to the waveform output display
	app_wavegen_render_setup();

	// Reset the QEI encoder position counter
	int32_t last_position_qei = 0;
	qei_reset_step();

	// Setup the analog switch that controls speaker/DACOUT
	GPIOSetDir(DAC1EN_PIN/32, DAC1EN_PIN%32, 1);
	if (_app_wavegen_output_spkr)
	{
		// Speaker output
		LPC_GPIO_PORT->SET0 = (1 << DAC1EN_PIN);
	}
	else
	{
		// DAC1 output
		LPC_GPIO_PORT->CLR0 = (1 << DAC1EN_PIN);
	}

	// Wait for the QEI switch to exit
	while (!(button_pressed() & (1 << QEI_SW_PIN)))
	{
		// Check for a scroll request on the QEI
		int32_t abs = qei_abs_step();
		// Adjust waveform offset on qei scroll
		if (abs != last_position_qei)
		{
			int16_t pos = (int16_t)_app_wavegen_curwave;
			pos += (abs - last_position_qei);
			if (pos >= APP_WAVEGEN_WAVE_LAST)
			{
				pos = APP_WAVEGEN_WAVE_SINE;
			}
			if (pos < 0)
			{
				pos = APP_WAVEGEN_WAVE_USER;
			}
			_app_wavegen_curwave = (app_wavegen_wave_t)pos;
			app_wavegen_render_setup();
			ssd1306_refresh();
			last_position_qei = abs;
		}
	}

	dac_wavegen_stop(WAVEGEN_DAC);
}
