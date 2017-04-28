/*
 * adc_dma.c
 *
 *  Created on: Apr 26, 2017
 *      Author: hathach
 */

#include <stdio.h>

#include "LPC8xx.h"
#include "lpc_types.h"
#include "core_cm0plus.h"
#include "lpc8xx_syscon.h"
#include "lpc8xx_swm.h"
#include "lpc8xx_adc.h"
#include "lpc8xx_dma.h"

#include "adc_dma.h"

// Max transfer count by DMA
uint16_t adc_buffer[DMA_BUFFER_SIZE];

// Instantiate the channel descriptor table, which must be 512-byte aligned (see lpc8xx_dma.h)
ALIGN(512) DMA_CHDESC_T Chan_Desc_Table[NUM_DMA_CHANNELS];

// Instantiate one reload descriptor. All descriptors must be 16-byte aligned (see lpc8xx_dma.h)
//ALIGN(16) DMA_RELOADDESC_T Reload_Descriptor_B;

uint8_t _channel = 6;

void adc_dma_init(void)
{
	/*------------- ADC -------------*/

	// Power up and reset the ADC, and enable clocks to peripherals
  LPC_SYSCON->PDRUNCFG &= ~(ADC_PD);		// Power up the ADC
  LPC_SYSCON->PRESETCTRL &= (ADC_RST_N);	// Reset the ADC
  LPC_SYSCON->PRESETCTRL |= ~(ADC_RST_N);
  LPC_SYSCON->SYSAHBCLKCTRL |= (ADC | GPIO | SWM );		// Enabled the ADC

  // Perform a self-calibration
  LPC_ADC->CTRL = (1 << ADC_CALMODE) | (0 << ADC_LPWRMODE) | (0 << ADC_CLKDIV);

  // Poll the calibration mode bit until it is cleared
  while ( LPC_ADC->CTRL & (1 << ADC_CALMODE) ) {}


  // This function configures ADC6 (P0_20) on the LPC824,
	// which corresponds to pin A5 on the Arduino headers.
	// Configure the SWM (see utilities_lib and lpc8xx_swm.h)
//	LPC_IOCON->PIO0_20 = 1<<7;	 			// No pull-up/down
	LPC_SWM->PINENABLE0 &= ~(ADC_6);	// Enable ADC on P0_20

  // Configure clock div
  LPC_ADC->CTRL = (0 << ADC_CALMODE) | (0 << ADC_LPWRMODE) | (0 << ADC_CLKDIV);

  // Configure the ADC for the appropriate analog supply voltage using the TRM register
  // For a sampling rate higher than 1 Msamples/s, VDDA must be higher than 2.7 V (on the Max board it is 3.3 V)
  LPC_ADC->TRM &= ~(1 << ADC_VRANGE);  // '0' for high voltage

	// Write the sequence control word with enable bit set for both sequences
	LPC_ADC->SEQA_CTRL = 1   << ADC_TRIGPOL | // Trigger pos edge
                       0   << ADC_TRIGGER | // SW trigger
                       1   << ADC_MODE    | // End of sequence
                       1   << _channel;         // Select channel

	LPC_ADC->SEQB_CTRL = 0x00; // not used

	// Clear Interrupt Flags
	LPC_ADC->FLAGS = LPC_ADC->FLAGS;

	// Enable Sequence interrupt to trigger DMA
	LPC_ADC->INTEN = 1 << SEQA_INTEN;

  // Enable Sequencer
  LPC_ADC->SEQA_CTRL |= (1UL << ADC_SEQ_ENA);

	/*------------- DMA -------------*/

	// Reset the DMA, and enable clocks to peripherals
  LPC_SYSCON->PRESETCTRL &= (DMA_RST_N);
  LPC_SYSCON->PRESETCTRL |= ~(DMA_RST_N);
  LPC_SYSCON->SYSAHBCLKCTRL |= DMA;

  // Use ADC Seq A to trigger DMA0
  LPC_DMATRIGMUX->DMA_ITRIG_INMUX0 = 0;

  // Point the SRAMBASE register to the beginning of the channel descriptor SRAM table
  LPC_DMA->SRAMBASE = (uint32_t)(&Chan_Desc_Table);

  // Enable the channel 0 interrupt in the INTEN register
  LPC_DMA->INTA0 = LPC_DMA->INTA0;
  LPC_DMA->INTB0 = LPC_DMA->INTB0;
  LPC_DMA->INTENSET0 =  1<<0;

  // Set the master DMA controller enable bit in the CTRL register
  LPC_DMA->CTRL = 1;

  // Set the valid bit for channel 0 in the SETVALID register
  LPC_DMA->SETVALID0 = 1<<0;

  // Enable DMA channel 0 in the ENABLE register
  LPC_DMA->ENABLESET0 = 1<<0;

  // Config Channel
  LPC_DMA->CHANNEL[0].CFG = 1<<DMA_CFG_HWTRIGEN     | // Hw trigger by ADC
                            1<<DMA_CFG_TRIGPOL      | //
                            0<<DMA_CFG_TRIGTYPE     |
                            1<<DMA_CFG_TRIGBURST    | // burst mode required
                            0<<DMA_CFG_BURSTPOWER   | // burst size in power of 2
                            0<<DMA_CFG_SRCBURSTWRAP | // TODO multiple ADC
                            0<<DMA_CFG_DSTBURSTWRAP |
                            0<<DMA_CFG_CHPRIORITY;

  // Config transfer
  uint32_t xfercfg = 1<<DMA_XFERCFG_CFGVALID |
                     0<<DMA_XFERCFG_RELOAD   | // multiple descriptor if need buffer > 1024
                     0<<DMA_XFERCFG_SWTRIG   |
                     1<<DMA_XFERCFG_CLRTRIG  |
                     1<<DMA_XFERCFG_SETINTA  |
                     0<<DMA_XFERCFG_SETINTB  |
                     1<<DMA_XFERCFG_WIDTH    | // 16 bit of ADC value
                     0<<DMA_XFERCFG_SRCINC   | // TODO multiple ADC
                     1<<DMA_XFERCFG_DSTINC   |
                     (DMA_BUFFER_SIZE-1) << DMA_XFERCFG_XFERCOUNT; // TODO multiple ADC

  // Channel Descriptor
  Chan_Desc_Table[0].source = (uint32_t) &LPC_ADC->DAT[_channel];
  Chan_Desc_Table[0].dest = (uint32_t) &adc_buffer[DMA_BUFFER_SIZE-1];
  Chan_Desc_Table[0].next = 0;

  // Enable the DMA interrupt in the NVIC
  NVIC_EnableIRQ(DMA_IRQn);

  LPC_DMA->CHANNEL[0].XFERCFG = xfercfg;
}


void adc_dma_start(void)
{
  // Use DMA only burst is supported
//  LPC_ADC->SEQA_CTRL |= (1UL << ADC_SEQ_ENA); // | (1 << ADC_BURST);
//  LPC_ADC->SEQA_CTRL |= (1 << ADC_BURST);
  LPC_ADC->SEQA_CTRL |= (1 << ADC_START);
}

void DMA_IRQHandler(void)
{
//  uint32_t intsts = LPC_DMA->INTA0; // Get the interrupt A flags
//
//  // CH0 is active
//  if ( intsts & 1UL )
//  {
//
//  }
//
//  LPC_DMA->INTA0 = intsts; // clear interrupt
}
