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

#define ADC_CHANNEL 3
#define ADC_SAMPLE_RATE 1200000

// Max transfer count by DMA
uint16_t adc_buffer[2*DMA_BUFFER_SIZE];

// Instantiate the channel descriptor table, which must be 512-byte aligned (see lpc8xx_dma.h)
ALIGN(512) DMA_CHDESC_T Chan_Desc_Table[NUM_DMA_CHANNELS];

// Instantiate one reload descriptor. All descriptors must be 16-byte aligned (see lpc8xx_dma.h)
ALIGN(16) DMA_RELOADDESC_T dma2ndDesc;

uint8_t _channel = 3;

void adc_dma_init(void)
{
	/*------------- ADC -------------*/

	// Power up and reset the ADC, and enable clocks to peripherals
  LPC_SYSCON->PDRUNCFG &= ~(ADC_PD);		// Power up the ADC
  LPC_SYSCON->SYSAHBCLKCTRL |= (ADC);		// Enabled the ADC
  LPC_SYSCON->PRESETCTRL &= (ADC_RST_N);	// Reset the ADC
  LPC_SYSCON->PRESETCTRL |= ~(ADC_RST_N);

  // Perform a self-calibration
  LPC_ADC->CTRL = (1 << ADC_CALMODE) | (0 << ADC_LPWRMODE) | (0 << ADC_CLKDIV);

  // Poll the calibration mode bit until it is cleared
  while ( LPC_ADC->CTRL & (1 << ADC_CALMODE) ) {}

  // Configure clock div
  LPC_ADC->CTRL = (0 << ADC_CALMODE) | (0 << ADC_LPWRMODE) | (0 << ADC_CLKDIV);

  // Configure the ADC for the appropriate analog supply voltage using the TRM register
  // For a sampling rate higher than 1 Msamples/s, VDDA must be higher than 2.7 V (on the Max board it is 3.3 V)
  LPC_ADC->TRM &= ~(1 << ADC_VRANGE);  // '0' for high voltage

	// Write the sequence control word with enable bit set for both sequences
	LPC_ADC->SEQA_CTRL = //3  << ADC_TRIGGER | // SCT0_OUT3 trigger if using SCT to trigger
                       0   << ADC_TRIGGER | // SW trigger, if using hw timer
                       1   << ADC_MODE    | // End of sequence
                       1   << _channel;     // Select channel

	LPC_ADC->SEQB_CTRL = 0x00; // not used

  // Configures ADC6 (P0_20) on the LPC824,
	// which corresponds to pin A5 on the Arduino headers.
	// Configure the SWM (see utilities_lib and lpc8xx_swm.h)
  LPC_SYSCON->SYSAHBCLKCTRL |= SWM;
//	LPC_IOCON->PIO0_20 = 1<<7;	 			// No pull-up/down
	LPC_SWM->PINENABLE0 &= ~(ADC_3);	// Enable ADC on P0_20

	// Clear Interrupt Flags
	LPC_ADC->FLAGS |= LPC_ADC->FLAGS;

	// Enable Sequence interrupt to trigger DMA
	LPC_ADC->INTEN = (1 << SEQA_INTEN);

  // Enable Sequencer
  LPC_ADC->SEQA_CTRL |= (1UL << ADC_SEQ_ENA);

	/*------------- DMA -------------*/

	// Reset the DMA, and enable clocks to peripherals
  LPC_SYSCON->PRESETCTRL &= (DMA_RST_N);
  LPC_SYSCON->PRESETCTRL |= ~(DMA_RST_N);
  LPC_SYSCON->SYSAHBCLKCTRL |= DMA;

  // Set the master DMA controller enable bit in the CTRL register
  LPC_DMA->CTRL = 1;

  // Point the SRAMBASE register to the beginning of the channel descriptor SRAM table
  LPC_DMA->SRAMBASE = (uint32_t)(&Chan_Desc_Table);

  // Enable DMA channel 0 in the ENABLE register
  LPC_DMA->ENABLESET0 = 1<<0;

  // Enable the channel 0 interrupt in the INTEN register
  LPC_DMA->INTA0 |= LPC_DMA->INTA0;
  LPC_DMA->INTB0 |= LPC_DMA->INTB0;
  LPC_DMA->INTENSET0 =  1<<0;

  // Config Channel
  LPC_DMA->CHANNEL[0].CFG = 1<<DMA_CFG_HWTRIGEN     | // Hw trigger by ADC
                            0<<DMA_CFG_TRIGTYPE     |
                            1<<DMA_CFG_TRIGPOL      | //
                            1<<DMA_CFG_TRIGBURST    | // burst mode required
                            0<<DMA_CFG_BURSTPOWER   | // burst size in power of 2
                            0<<DMA_CFG_SRCBURSTWRAP | // TODO multiple ADC
                            0<<DMA_CFG_DSTBURSTWRAP |
                            0<<DMA_CFG_CHPRIORITY;

  // Use ADC Seq A to trigger DMA0
  LPC_DMATRIGMUX->DMA_ITRIG_INMUX0 = 0;

  // Enable the DMA interrupt in the NVIC
  NVIC_EnableIRQ(DMA_IRQn);

#if 0 // trigger using SCT,
  /*------------- SCT -------------*/
  LPC_SYSCON->SYSAHBCLKCTRL |= SCT;
  LPC_SYSCON->PRESETCTRL &= (SCT0_RST_N);
  LPC_SYSCON->PRESETCTRL |= ~(SCT0_RST_N);

	// Match/capture mode register. (ref UM10800 section 16.6.11, Table 232, page 273)
	// Determines if match/capture operate as match or capture. Want all match.
	LPC_SCT->REGMODE = 0;

	// Event 0 control: (ref UM10800 section 16.6.25, Table 247, page 282).
	// set MATCHSEL (bits 3:0) = MATCH0 register(0)
	// set COMBMODE (bits 13:12)= MATCH only(1)
	// So Event0 is triggered on match of MATCH0
	LPC_SCT->EVENT[0].CTRL =   (0 << 0 ) | 1 << 12;

	// Event enable register (ref UM10800 section 16.6.24, Table 246, page 281)
	// Enable Event0 in State0 (default state). We are not using states,
	// so this enables Event0 in the default State0.
	// Set STATEMSK0=1
	LPC_SCT->EVENT[0].STATE = 1<<0;

	// Configure Event2 to be triggered on Match2
	// The match register (MATCH2) associated with this event
	// COMBMODE=1 (MATCH only)
	LPC_SCT->EVENT[2].CTRL = (2 << 0) | (1 << 12);
	LPC_SCT->EVENT[2].STATE = 1; // Enable Event2 in State0 (default state)

	/* Clear the output in-case of conflict */
	int pin = 0;
	LPC_SCT->RES = (LPC_SCT->RES & ~(3 << (pin << 1))) | (0x01 << (pin << 1));

	/* Set and Clear do not depend on direction */
	LPC_SCT->OUTPUTDIRCTRL = (LPC_SCT->OUTPUTDIRCTRL & ~((3 << (pin << 1))| (~0xff)));

	// Set SCT Counter to count 32-bits and reset to 0 after reaching MATCH0
	LPC_SCT->CONFIG = 0x00000001 | (0x1 << 17);


	// Setup SCT for ADC/DMA sample timing.
	// Setup SCT for ADC/DMA sample timing.
	LPC_SCT->MATCHREL[2].U = (SystemCoreClock/ADC_SAMPLE_RATE)/2;
	LPC_SCT->MATCHREL[0].U = SystemCoreClock/ADC_SAMPLE_RATE;

	// Using SCT0_OUT3 to trigger ADC sampling
	// Set SCT0_OUT3 on Event0 (Event0 configured to occur on Match0)
	LPC_SCT->OUT[3].SET = 1 << 0;
	// Clear SCT0_OUT3 on Event2 (Event2 configured to occur on Match2)
	LPC_SCT->OUT[3].CLR = 1 << 2;

	// SwitchMatrix: Assign SCT_OUT3 to external pin for debugging
//	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
//	Chip_SWM_MovablePinAssign(SWM_SCT_OUT3_O, PIN_SCT_DEBUG);
//	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);

	// Start SCT
	LPC_SCT->CTRL &= ~((1 << 2) | (1 << 18));
#endif
}

void adc_dma_set_rate(uint32_t period_us)
{
  // Set up systick to trigger ADC sampling
  SysTick_Config( (SystemCoreClock/1000000) * period_us );

  // disable systick, call adc_dma_start() to sample
  NVIC_DisableIRQ(SysTick_IRQn);
}

void adc_dma_start(void)
{
  // DMA in action, wait until it is done
  while ( LPC_DMA->CHANNEL[0].XFERCFG & (1<<DMA_XFERCFG_CFGVALID) ) {}

  uint32_t xfercfg = 1<<DMA_XFERCFG_CFGVALID |
                     //1<<DMA_XFERCFG_CLRTRIG  |
                     1<<DMA_XFERCFG_SETINTA  |
                     0<<DMA_XFERCFG_SETINTB  |
                     1<<DMA_XFERCFG_WIDTH    | // 16 bit of ADC value
                     0<<DMA_XFERCFG_SRCINC   | // TODO multiple ADC
                     1<<DMA_XFERCFG_DSTINC   |
                     (DMA_BUFFER_SIZE-1) << DMA_XFERCFG_XFERCOUNT; // TODO multiple ADC

  // Set the valid bit for channel 0 in the SETVALID register
  LPC_DMA->SETVALID0 = 1<<0;

  // 2nd descriptor.
  dma2ndDesc.xfercfg = xfercfg; // last descriptor has no reload bit set
  dma2ndDesc.source  = (uint32_t) &LPC_ADC->DAT[_channel];
  dma2ndDesc.dest    = (uint32_t) &adc_buffer[2*DMA_BUFFER_SIZE-1];
  dma2ndDesc.next    = 0;

  // Channel Descriptor, the first descriptor
  Chan_Desc_Table[0].source = (uint32_t) &LPC_ADC->DAT[_channel];
  Chan_Desc_Table[0].dest   = (uint32_t) &adc_buffer[DMA_BUFFER_SIZE-1];
  Chan_Desc_Table[0].next   = (uint32_t) &dma2ndDesc; // link 2nd descriptor to chanel descriptor

  // Set XferCfg register, this will put DMA into ready mode
  // Set Reload bit, this will cause DMA controller to fetch the 2nd descriptor
  // in the next link.
  LPC_DMA->CHANNEL[0].XFERCFG = xfercfg | 1<<DMA_XFERCFG_RELOAD;

  // enable systick to sample ADC
  NVIC_EnableIRQ(SysTick_IRQn);
}

void adc_dma_stop(void)
{
  // DMA in action, wait until it is done
  while ( LPC_DMA->CHANNEL[0].XFERCFG & (1<<DMA_XFERCFG_CFGVALID) ) {}

  // enable systick to sample ADC
  NVIC_DisableIRQ(SysTick_IRQn);
}

void SysTick_Handler(void)
{
  // sample using START bit
  LPC_ADC->SEQA_CTRL |= (1 << ADC_START);
}

void DMA_IRQHandler(void)
{
  // when a Descriptor 1024 completed, this ISR is called
  // Currently there is 2 DMA linked to each other. Therefore
  // This function will be called two times to complete 2K buffer

  uint32_t intsts = LPC_DMA->INTA0; // Get the interrupt A flags

  // CH0 is active
  if ( intsts & 1UL )
  {

  }

  LPC_DMA->INTA0 = intsts; // clear interrupt
}
