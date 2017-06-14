/*
===============================================================================
 Name        : gfx_tester.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Misc test functions for the display sub-system
===============================================================================
*/


#include "../gfx.h"
#include "../delay.h"

static const uint16_t gfx_tester_sine_wave[64] = {
	0x200, 0x232, 0x263, 0x294, 0x2c3, 0x2f1, 0x31c, 0x344,
	0x369, 0x38b, 0x3a9, 0x3c3, 0x3d8, 0x3e9, 0x3f5, 0x3fd,
	0x3ff, 0x3fd, 0x3f5, 0x3e9, 0x3d8, 0x3c3, 0x3a9, 0x38b,
	0x369, 0x344, 0x31c, 0x2f1, 0x2c3, 0x294, 0x263, 0x232,
	0x200, 0x1cd, 0x19c, 0x16b, 0x13c, 0x10e, 0x0e3, 0x0bb,
	0x096, 0x074, 0x056, 0x03c, 0x027, 0x016, 0x00a, 0x002,
	0x000, 0x002, 0x00a, 0x016, 0x027, 0x03c, 0x056, 0x074,
	0x096, 0x0bb, 0x0e3, 0x10e, 0x13c, 0x16b, 0x19c, 0x1cd
};

static const uint16_t gfx_tester_triangle_wave[64] = {
	0x020, 0x040, 0x060, 0x080, 0x0a0, 0x0c0, 0x0e0, 0x100,
	0x120, 0x140, 0x160, 0x180, 0x1a0, 0x1c0, 0x1e0, 0x200,
	0x21f, 0x23f, 0x25f, 0x27f, 0x29f, 0x2bf, 0x2df, 0x2ff,
	0x31f, 0x33f, 0x35f, 0x37f, 0x39f, 0x3bf, 0x3df, 0x3ff,
	0x3df, 0x3bf, 0x39f, 0x37f, 0x35f, 0x33f, 0x31f, 0x2ff,
	0x2df, 0x2bf, 0x29f, 0x27f, 0x25f, 0x23f, 0x21f, 0x200,
	0x1e0, 0x1c0, 0x1a0, 0x180, 0x160, 0x140, 0x120, 0x100,
	0x0e0, 0x0c0, 0x0a0, 0x080, 0x060, 0x040, 0x020, 0x000
};

static const uint16_t gfx_tester_expdecay_wave[64] = {
	0x3ff, 0x3c1, 0x387, 0x350, 0x31d, 0x2ec, 0x2bf, 0x294,
	0x26c, 0x247, 0x224, 0x202, 0x1e3, 0x1c6, 0x1aa, 0x191,
	0x178, 0x162, 0x14c, 0x138, 0x125, 0x113, 0x103, 0x0f3,
	0x0e4, 0x0d6, 0x0c9, 0x0bd, 0x0b2, 0x0a7, 0x90d, 0x093,
	0x08a, 0x082, 0x07a, 0x073, 0x06c, 0x065, 0x05f, 0x059,
	0x054, 0x04f, 0x04a, 0x046, 0x041, 0x03d, 0x03a, 0x036,
	0x033, 0x030, 0x02d, 0x02a, 0x028, 0x025, 0x023, 0x021,
	0x01f, 0x01d, 0x01b, 0x01a, 0x018, 0x017, 0x015, 0x014
};

static int gfx_tester_scope(const uint16_t *waveform, uint8_t offset)
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
    ssd1306_set_text(127-48, 0, 1, "WAVEFORM", 1);	// 48 pixels wide

	// Render the graticule
	gfx_graticule(0, 16, &grcfg, 1);

	// Render a sample sine wave
	gfx_waveform_64_32(0, 16, 1, waveform, offset, 0);

	// Render some labels
	gfx_printdec(70, 16, waveform[(offset + 32) % 64], 1, 1);
    ssd1306_set_text(70, 16, 1, "    mV", 1);
    ssd1306_set_text(70, 35, 1, "200 ms/div", 1);
    ssd1306_set_text(70, 43, 1, "100 mV/div", 1);

    // Render the bottom button options
	ssd1306_fill_rect(0, 55, 128, 8, 1);
    ssd1306_set_text(8, 56, 0, "HOME", 1);
    ssd1306_set_text(36, 56, 0, "TIME", 1);
    ssd1306_set_text(64, 56, 0, "TRIG", 1);
    ssd1306_set_text(96, 56, 0, "STOP", 1);

    ssd1306_refresh();

    return 0;
}

static int gfx_tester_wavegen(const uint16_t *waveform)
{
	gfx_graticule_cfg_t grcfg =
	{
		.w = 64,			// 64 pixels wide
		.h = 32,			// 32 pixels high
		.lines = GFX_GRATICULE_LINES_BOT | GFX_GRATICULE_LINES_TOP,
		.line_spacing = 2,	// Divider lines are 1 dot every 2 pixels
		.block_spacing = 8	// Each block is 8x8 pixels
	};

	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-42, 0, 1, "WAVEGEN", 1);	// 42 pixels wide

	// Render the graticule
	gfx_graticule(0, 16, &grcfg, 1);

	// Render a sample sine wave
	gfx_waveform_64_32(0, 16, 1, waveform, 0, 0);

	// Render some labels
    ssd1306_set_text(70, 16, 1, "WFRM SINE", 1);
    ssd1306_set_text(70, 24, 1, "FREQ 200 Hz", 1);
    ssd1306_set_text(70, 32, 1, "AMPL 3.3 V", 1);
    ssd1306_set_text(78, 44, 1, "RUNNING", 1);

    // Render the bottom button options
	ssd1306_fill_rect(0, 55, 127, 8, 1);
    ssd1306_set_text(8, 56, 0, "HOME", 1);
    ssd1306_set_text(36, 56, 0, "WFRM", 1);
    ssd1306_set_text(64, 56, 0, "FREQ", 1);
    ssd1306_set_text(96, 56, 0, "AMPL", 1);

    ssd1306_refresh();

    return 0;
}

static int gfx_tester_i2c_scanner(void)
{
	uint8_t i;

	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-42, 0, 1, "I2CSCAN", 1);	// 42 pixels wide

    // Render the bottom button options
	ssd1306_fill_rect(0, 55, 127, 8, 1);
    ssd1306_set_text(8, 56, 0, "HOME", 1);
    ssd1306_set_text(36, 56, 0, "", 1);
    ssd1306_set_text(64, 56, 0, "LSTN", 1);
    ssd1306_set_text(96, 56, 0, "SCAN", 1);

    // Render the static address label text
    ssd1306_set_text(6, 8, 1, "SCANNING I2C ADDR 0x", 1);

    ssd1306_refresh();

    // Scan all valid I2C addresses (0x08..0x77)
	for (i=0x08; i<0x78; i++)
	{
		ssd1306_fill_rect(106, 8, 12, 8, 0);
	    gfx_printhex8(106, 8, i, 1, 1);

	    // Simulate some matches
	    if (i == 0x14)
	    {
		    gfx_printhex8(0, 28, i, 2, 1);
	    }
	    if (i == 0x27)
	    {
		    gfx_printhex8(24, 28, i, 2, 1);
	    }
	    if (i == 0x42)
	    {
		    gfx_printhex8(48, 28, i, 2, 1);
	    }
	    if (i == 0x6A)
	    {
		    gfx_printhex8(72, 28, i, 2, 1);
	    }

	    ssd1306_refresh();
	}

    return 0;
}

static int gfx_tester_voltmeter(void)
{
	uint8_t i;

	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "NXP SAKEE", 1);
    ssd1306_set_text(127-54, 0, 1, "VOLTMETER", 1);	// 54 pixels wide

    // Render the bottom button options
	ssd1306_fill_rect(0, 55, 127, 8, 1);
    ssd1306_set_text(8, 56, 0, "HOME", 1);
    ssd1306_set_text(36, 56, 0, "", 1);
    ssd1306_set_text(64, 56, 0, "UNIT", 1);
    ssd1306_set_text(96, 56, 0, "STOP", 1);

    ssd1306_set_text(90, 32, 1, "VOLTS", 1);

    ssd1306_refresh();

    // Simulate some flicker on the output
    ssd1306_set_text(20, 20, 1, "2.", 3);
    for (i=0; i<100; i++)
    {
		ssd1306_fill_rect(48, 20, 36, 24, 0);

        switch (i%5)
        {
        case 0:
            ssd1306_set_text(48, 20, 1, "77", 3);
        	break;
        case 1:
            ssd1306_set_text(48, 20, 1, "80", 3);
        	break;
        case 2:
            ssd1306_set_text(48, 20, 1, "78", 3);
        	break;
        default:
            ssd1306_set_text(48, 20, 1, "79", 3);
        	break;
        }

        ssd1306_refresh();
    	delay_ms(50);
    }

    return 0;
}

int gfx_tester_run(void)
{
	uint8_t i;

	// Render a scrolling sine wave
	for (i=0; i<64; i++)
	{
		gfx_tester_scope(gfx_tester_sine_wave, i);
	}
	delay_ms(4000);

	// Render wavegen
	gfx_tester_wavegen(gfx_tester_expdecay_wave);
	delay_ms(5000);

	// Render the I2C scanner
	gfx_tester_i2c_scanner();
	delay_ms(3000);

	// Render the voltmeter
	gfx_tester_voltmeter();
	delay_ms(3000);

	return 0;
}
