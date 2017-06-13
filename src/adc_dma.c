/*
 ===============================================================================
 Name        : adc_dma.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Systick and DMA based ADC sampler
 ===============================================================================
 */

#include <stdio.h>

#include "LPC8xx.h"
#include "lpc_types.h"
#include "core_cm0plus.h"
#include "syscon.h"
#include "swm.h"
#include "adc.h"
#include "dma.h"

#include "adc_dma.h"

/*
 Pins used in this application:

 P0.23 [I] - ADC3 (A2 on Arduino Header)
*/

// Buffer with max number of samples to store via DMA
// Should be a multiple of 1024 (DMA_BUFFER_SIZE)
uint16_t adc_buffer[2 * DMA_BUFFER_SIZE];

// Instantiate the channel descriptor table, which must be 512-byte aligned (see lpc8xx_dma.h)
ALIGN(512) DMA_CHDESC_T Chan_Desc_Table[NUM_DMA_CHANNELS];

// Instantiate one reload descriptor. All descriptors must be 16-byte aligned (see lpc8xx_dma.h)
ALIGN(16) DMA_RELOADDESC_T dma2ndDesc;

// ADC channel to use
const uint8_t _channel = 2;
#define ADC_N(_n)    (1 << (14+(_n)))


void adc_dma_init(void)
{
	/*------------- ADC -------------*/
	// Power up and reset the ADC, enable clocks to peripherals
	LPC_SYSCON->PDRUNCFG &= ~(ADC_PD);		// Power up the ADC
	LPC_SYSCON->SYSAHBCLKCTRL0 |= (ADC);		// Enable the ADC
	LPC_SYSCON->PRESETCTRL0 &= (ADC_RST_N);	// Reset the ADC
	LPC_SYSCON->PRESETCTRL0 |= ~(ADC_RST_N);

	// Perform a self-calibration
	LPC_ADC->CTRL = (1 << ADC_CALMODE)  |
			            (0 << ADC_LPWRMODE) |
			            (0 << ADC_CLKDIV);

	// Poll the calibration mode bit until it is cleared
	while ( LPC_ADC->CTRL & (1 << ADC_CALMODE) ) { }

	// Configure clock div
	LPC_ADC->CTRL = (0 << ADC_CALMODE)  |
			            (0 << ADC_LPWRMODE) |
			            (0 << ADC_CLKDIV);

	// Configure the ADC for the appropriate analog supply voltage using the
	// TRM register. For a sampling rate higher than 1 Msamples/s, VDDA must
	// be higher than 2.7 V (on the Max board it is 3.3 V)
	LPC_ADC->TRM &= ~(1 << ADC_VRANGE);  // '0' for high voltage

	// Write the sequence control word
	LPC_ADC->SEQA_CTRL = 0 << ADC_TRIGGER | // SW trigger, if using hw timer
					             1 << ADC_MODE    | // End of sequence
					             1 << _channel;     // Select channel

	// Sequence B not used
	LPC_ADC->SEQB_CTRL = 0x00;

	// Configure the SWM (see utilities_lib and lpc8xx_swm.h)
	LPC_SYSCON->SYSAHBCLKCTRL0 |= SWM;

	// Configure ADC pin on the LPC845,
	LPC_SWM->PINENABLE0 &= ~(ADC_N(_channel));

	// Clear Interrupt Flags
	LPC_ADC->FLAGS |= LPC_ADC->FLAGS;

	// Enable sequence A interrupt to trigger DMA transfer
	LPC_ADC->INTEN = (1 << SEQA_INTEN);

	// Enable Sequence A
	LPC_ADC->SEQA_CTRL |= (1UL << ADC_SEQ_ENA);

	/*------------- DMA -------------*/

	// Reset the DMA, and enable peripheral clocks
	LPC_SYSCON->PRESETCTRL0 &= (DMA_RST_N);
	LPC_SYSCON->PRESETCTRL0 |= ~(DMA_RST_N);
	LPC_SYSCON->SYSAHBCLKCTRL0 |= DMA;

	// Set the master DMA controller enable bit in the CTRL register
	LPC_DMA->CTRL = 1;

	// Point the SRAMBASE register to the beginning of the channel descriptor SRAM table
	LPC_DMA->SRAMBASE = (uint32_t) (&Chan_Desc_Table);

	// Enable DMA channel 0 in the ENABLE register
	LPC_DMA->ENABLESET0 = 1 << 0;

	// Enable the channel 0 interrupt in the INTEN register
	LPC_DMA->INTA0 |= LPC_DMA->INTA0;
	LPC_DMA->INTB0 |= LPC_DMA->INTB0;
	LPC_DMA->INTENSET0 = 1 << 0;

	// Configure the DMA channel
	LPC_DMA->CHANNEL[0].CFG = 1 << DMA_CFG_HWTRIGEN |		// HW triggered by ADC
			                  0 << DMA_CFG_TRIGTYPE |
			                  1 << DMA_CFG_TRIGPOL |
			                  1 << DMA_CFG_TRIGBURST |		// Burst mode required
			                  0 << DMA_CFG_BURSTPOWER | 	// Burst size in power of 2
			                  0 << DMA_CFG_SRCBURSTWRAP | 	// TODO: Multiple ADC
			                  0 << DMA_CFG_DSTBURSTWRAP |
			                  0 << DMA_CFG_CHPRIORITY;

	// Use ADC Seq A to trigger DMA0
	LPC_INMUX_TRIGMUX->DMA_ITRIG_INMUX0 = 0;

	// Enable the DMA interrupt in the NVIC
	NVIC_EnableIRQ(DMA_IRQn);
}

void adc_dma_set_rate(uint32_t period_us)
{
	// Setup systick timer to set the ADC sampling rate
	SysTick_Config((system_ahb_clk / 1000000) * period_us);

	// Disable systick timer, call adc_dma_start() to start sampling
	NVIC_DisableIRQ(SysTick_IRQn);
}

void adc_dma_start(void)
{
	// If DMA is busy, wait until it is finished
	while ( LPC_DMA->CHANNEL[0].XFERCFG & (1 << DMA_XFERCFG_CFGVALID))
	{
	}

	// DMA transfer config
	uint32_t xfercfg = 1 << DMA_XFERCFG_CFGVALID |
					   // 1 << DMA_XFERCFG_CLRTRIG |
					   1 << DMA_XFERCFG_SETINTA |
					   0 << DMA_XFERCFG_SETINTB |
					   1 << DMA_XFERCFG_WIDTH | 	// 16 bits for 12-bit ADC values
					   0 << DMA_XFERCFG_SRCINC | 	// TODO multiple ADC
					   1 << DMA_XFERCFG_DSTINC |
					   (DMA_BUFFER_SIZE - 1) << DMA_XFERCFG_XFERCOUNT;

	// Set the valid bit for channel 0 in the SETVALID register
	LPC_DMA->SETVALID0 = 1 << 0;

	// Second descriptor
	dma2ndDesc.xfercfg = xfercfg; // last descriptor has no reload bit set
	dma2ndDesc.source = (uint32_t) &LPC_ADC->DAT[_channel];
	dma2ndDesc.dest = (uint32_t) &adc_buffer[2 * DMA_BUFFER_SIZE - 1];
	dma2ndDesc.next = 0;

	// Channel Descriptor, the first descriptor
	Chan_Desc_Table[0].source = (uint32_t) &LPC_ADC->DAT[_channel];
	Chan_Desc_Table[0].dest = (uint32_t) &adc_buffer[DMA_BUFFER_SIZE - 1];
	Chan_Desc_Table[0].next = (uint32_t) &dma2ndDesc; // link 2nd descriptor to channel descriptor

	// Set XFERCFG register, which will put DMA into ready mode
	// Set Reload bit, which will cause the DMA controller to fetch
	// the 2nd descriptor in the next link.
	LPC_DMA->CHANNEL[0].XFERCFG = xfercfg | 1 << DMA_XFERCFG_RELOAD;

	// Enable the systick timer to start samping the ADC at the specified sample rate
	NVIC_EnableIRQ(SysTick_IRQn);
}

void adc_dma_stop(void)
{
	// If DMA is busy, wait until it is finished
	while ( LPC_DMA->CHANNEL[0].XFERCFG & (1 << DMA_XFERCFG_CFGVALID))
	{
	}

	// Disable the systick timer and stop sampling the ADC
	NVIC_DisableIRQ(SysTick_IRQn);
}

void SysTick_Handler(void)
{
	// Get a new ADC sample using the START bit
	LPC_ADC->SEQA_CTRL |= (1 << ADC_START);
}

void DMA_IRQHandler(void)
{
	// When the DMA hits 1024, this ISR is called
	// Currently there are 2 1KB DMA transfers linked together.
	// Therefore this function will be called twice to fill the 2KB buffer

	uint32_t intsts = LPC_DMA->INTA0; // Get the interrupt A flag

	// CH0 is active
	if (intsts & 1UL)
	{
	}

	LPC_DMA->INTA0 = intsts; // Clear interrupt
}
