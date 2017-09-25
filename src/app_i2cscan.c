/*
===============================================================================
 Name        : app_i2cscan.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Main menu
===============================================================================
 */

#include "LPC8xx.h"
#include "i2c.h"
#include "swm.h"
#include "syscon.h"

#include "config.h"
#include "button.h"
#include "delay.h"
#include "app_i2cscan.h"
#include "gfx.h"

void app_i2cscan_init(void)
{
	ssd1306_clear();
    ssd1306_refresh();

	// Provide main_clk as function clock to I2C0
	LPC_SYSCON->I2C0CLKSEL = FCLKSEL_MAIN_CLK;

	// Enable bus clocks to I2C0, SWM
	LPC_SYSCON->SYSAHBCLKCTRL0 |= (I2C0 | SWM);

	// Configure the SWM
	LPC_SWM->PINENABLE0 &= ~(I2C0_SCL|I2C0_SDA);

	// Give I2C0 a reset
	LPC_SYSCON->PRESETCTRL0 &= (I2C0_RST_N);
	LPC_SYSCON->PRESETCTRL0 |= ~(I2C0_RST_N);

	// Configure the I2C0 clock divider
	// Desired bit rate = Fscl = 100,000 Hz (1/Fscl = 10 us, 5 us low and 5 us high)
	// Use default clock high and clock low times (= 2 clocks each)
	// So 4 I2C_PCLKs = 100,000/second, or 1 I2C_PCLK = 400,000/second
	// I2C_PCLK = SystemClock = 30,000,000/second, so we divide by 30000000/400000 = 75
	// Remember, value written to DIV divides by value+1
	SystemCoreClockUpdate(); // Get main_clk frequency
	LPC_I2C0->DIV = (main_clk/400000) - 1;

	// Configure the I2C0 CFG register:
	// Master enable = true
	// Slave enable = false
	// Monitor enable = false
	// Time-out enable = false
	// Monitor function clock stretching = false
	//
	LPC_I2C0->CFG = CFG_MSTENA;
}

int app_i2cscan_check_addr(uint8_t addr)
{
    // Wait for the master state to be idle
    while ((LPC_I2C0->STAT & MASTER_STATE_MASK) != I2C_STAT_MSTST_IDLE);

    // Try to communicate with the specified addr
    LPC_I2C0->MSTDAT = (addr<<1) | 0;                  // Address with 0 for RWn bit (WRITE)
    LPC_I2C0->MSTCTL = CTL_MSTSTART;                   // Start the transaction by setting the MSTSTART bit to 1 in the Master control register.

    // Wait for MSTPENDING bit to indicate that the request has been ACK'd
    uint8_t timeout = 0;
    while(!(LPC_I2C0->STAT & STAT_MSTPEND) && (timeout < 3))
    {
    	delay_ms(1);
    	timeout++;
    }

    if((LPC_I2C0->STAT & MASTER_STATE_MASK) != I2C_STAT_MSTST_TX) {
    	// Error!
	    LPC_I2C0->MSTCTL = CTL_MSTSTOP;                // Send a stop to end the transaction
		return -1;
    }

    // ACK received!
    LPC_I2C0->MSTCTL = CTL_MSTSTOP;                    // Send a stop to end the transaction

    return 1;
}

void app_i2cscan_run(void)
{
	uint8_t addr, dev_count;

	ssd1306_clear();

	// Render the title bars
    ssd1306_set_text(0, 0, 1, "LPC SAKEE", 1);
    ssd1306_set_text(127-66, 0, 1, "I2C SCANNER", 1);	// 66 pixels wide

    // Render the static address label text
    ssd1306_set_text(0, 8, 1, "SCANNING I2C ADDR: 0x", 1);

    // Render the bottom button options
	//ssd1306_fill_rect(0, 55, 127, 8, 1);
    //ssd1306_set_text(8, 56, 0, "HOME", 1);

    /* Scan all valid I2C addresses (0x08..0x77) */
    dev_count = 0;
    for (addr = 0x08; addr < 0x78; addr++)
    {
		ssd1306_fill_rect(105, 8, 12, 8, 0);
	    gfx_printhex8(105, 8, addr, 1, 1);
        ssd1306_refresh();

        /* Display the addr if a response was received */
        int rc = app_i2cscan_check_addr(addr);
        if (rc > 0)
        {
		    gfx_printhex8(dev_count*24, 25, addr, 2, 1);
            dev_count++;
		    ssd1306_refresh();
        }
    }

    if (dev_count < 10)
    {
        ssd1306_set_text(14, 48, 1, "I2C Devices Found: ", 1);
    	gfx_printdec(109, 48, (int32_t)dev_count, 1, 1);
    }
    else
    {
        ssd1306_set_text(11, 48, 1, "I2C Devices Found: ", 1);
    	gfx_printdec(106, 48, (int32_t)dev_count, 1, 1);
    }

	ssd1306_set_text(16, 55, 1, "CLICK FOR MAIN MENU", 1);

	ssd1306_refresh();

	/* Wait for the QEI switch to exit */
	while (!(button_pressed() &  ( 1 << QEI_SW_PIN)))
    {
    }
}
