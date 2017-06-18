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
#include "mrt.h"

#include "adc_dma.h"

// Buffer with max number of samples to store via DMA
// Should be a multiple of 1024 (DMA_BUFFER_SIZE)
uint16_t adc_buffer[2 * DMA_BUFFER_SIZE];

// Used to track the sample that caused the threshold interrupt to fire
volatile int16_t _adc_dma_trigger_offset;

uint32_t _adc_rate_us;

// Instantiate the channel descriptor table, which must be 512-byte aligned (see lpc8xx_dma.h)
ALIGN(512) DMA_CHDESC_T Chan_Desc_Table[NUM_DMA_CHANNELS];

// Instantiate one reload descriptor. All descriptors must be 16-byte aligned (see lpc8xx_dma.h)
ALIGN(16) DMA_RELOADDESC_T dma2ndDesc;

// ADC channel to use
// In this application it is P0.14 (A0)  Analog Input - ADC2
const uint8_t _channel = 2;
#define ADC_N(_n)    (1 << (14+(_n)))

static inline void enable_sample_timer(void)
{
  NVIC_EnableIRQ(MRT_IRQn);
}

static inline void disable_sample_timer(void)
{
  NVIC_DisableIRQ(MRT_IRQn);
}

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
  LPC_DMA->CHANNEL[0].CFG = 1 << DMA_CFG_HWTRIGEN     |		// HW triggered by ADC
                            0 << DMA_CFG_TRIGTYPE     |
                            1 << DMA_CFG_TRIGPOL      |
                            1 << DMA_CFG_TRIGBURST    |		// Burst mode required
                            0 << DMA_CFG_BURSTPOWER   | 	// Burst size in power of 2
                            0 << DMA_CFG_SRCBURSTWRAP | 	// TODO: Multiple ADC
                            0 << DMA_CFG_DSTBURSTWRAP |
                            0 << DMA_CFG_CHPRIORITY;

  // Use ADC Seq A to trigger DMA0
  LPC_INMUX_TRIGMUX->DMA_ITRIG_INMUX0 = 0;

  // Enable the DMA interrupt in the NVIC
  NVIC_EnableIRQ(DMA_IRQn);

  /*------------- MRT -------------*/

  // Setup MRT as sampling timer
  LPC_SYSCON->SYSAHBCLKCTRL0 |= MRT;

  LPC_SYSCON->PRESETCTRL0 &= (MRT_RST_N);
  LPC_SYSCON->PRESETCTRL0 |= ~(MRT_RST_N);

  // Mode = repeat, interrupt = enable
  LPC_MRT->Channel[0].CTRL = (MRT_Repeat<<MRT_MODE) | (1<<MRT_INTEN);
}

int adc_dma_set_rate(uint32_t period_us)
{
  // At 12MHz one tick is 80ns (0.08us), and a single ADC
  // transaction takes minimum 25 ticks so the minimum same rate
  // is 2us (0.08*25) or 500kHz.
  if (period_us < 2)
  {
	  return -1;
  }

  LPC_MRT->Channel[0].INTVAL = (system_ahb_clk / 1000000) * period_us;

  // Store value in us for later reference
  _adc_rate_us = period_us;

  // Disable sampling timer
  disable_sample_timer();

  return 0;
}

uint32_t adc_dma_get_rate(void)
{
	return _adc_rate_us;
}

void MRT_IRQHandler(void)
{
  // Get a new ADC sample using the START bit
  LPC_ADC->SEQA_CTRL |= (1 << ADC_START);

  // clear interrupt
  LPC_MRT->IRQ_FLAG |= LPC_MRT->IRQ_FLAG;
}

static void cfg_dma_xfer(uint32_t count)
{
  // currently only support count = 1024 or 2*1024
  uint32_t xfercfg = 1 << DMA_XFERCFG_CFGVALID |
                     // 1 << DMA_XFERCFG_CLRTRIG |
                     1 << DMA_XFERCFG_SETINTA |
                     0 << DMA_XFERCFG_SETINTB |
                     1 << DMA_XFERCFG_WIDTH | 	// 16 bits for 12-bit ADC values
                     0 << DMA_XFERCFG_SRCINC | 	// TODO multiple ADC
                     1 << DMA_XFERCFG_DSTINC |
                     (DMA_BUFFER_SIZE - 1) << DMA_XFERCFG_XFERCOUNT;

  if ( count > DMA_BUFFER_SIZE )
  {
    // Second descriptor
    dma2ndDesc.xfercfg = xfercfg; // last descriptor has no reload bit set
    dma2ndDesc.source  = (uint32_t) &LPC_ADC->DAT[_channel];
    dma2ndDesc.dest    = (uint32_t) &adc_buffer[2 * DMA_BUFFER_SIZE - 1];
    dma2ndDesc.next    = 0;
  }

  // Channel Descriptor, the first descriptor
  // link 2nd descriptor to channel descriptor if needed
  Chan_Desc_Table[0].source = (uint32_t) &LPC_ADC->DAT[_channel];
  Chan_Desc_Table[0].dest = (uint32_t) &adc_buffer[DMA_BUFFER_SIZE - 1];
  Chan_Desc_Table[0].next = ( count > DMA_BUFFER_SIZE ) ? ((uint32_t) &dma2ndDesc) : 0;

  // Set the valid bit for channel 0 in the SETVALID register
  LPC_DMA->SETVALID0 = 1 << 0;

  // Set XFERCFG register, which will put DMA into ready mode
  // Set Reload bit, which will cause the DMA controller to fetch
  // the 2nd descriptor in the next link.
  LPC_DMA->CHANNEL[0].XFERCFG = xfercfg | ((count > DMA_BUFFER_SIZE) ? (1 << DMA_XFERCFG_RELOAD) : 0);
}

void adc_dma_start(void)
{
  // If DMA is busy, wait until it is finished
  while ( adc_dma_busy() ) { }

  // Enable sequence A interrupt to trigger DMA transfer
  LPC_ADC->INTEN = (1 << SEQA_INTEN);
  cfg_dma_xfer(2*DMA_BUFFER_SIZE);

  // Start sampling using hardware timer
  enable_sample_timer();
}

/**
 *
 * @param low
 * @param high
 * @param mode 0 = disabled, 1 = outside threshold, 2 = crossing threshold
 */
void adc_dma_start_with_threshold(uint16_t low, uint16_t high, uint8_t mode)
{
  // If DMA is busy, wait until it is finished
  while ( adc_dma_busy() ) { }

  _adc_dma_trigger_offset = -1;

  // Threshold config
  LPC_ADC->THR0_LOW    = low << 4;
  LPC_ADC->THR0_HIGH   = high << 4;
  LPC_ADC->CHAN_THRSEL = (0 << _channel); // select threshold 0

  LPC_ADC->FLAGS |= (1 << _channel); // clear THCMP interrupt
  LPC_ADC->INTEN = (1 << SEQA_INTEN) | (mode << (3 + 2*_channel));
  NVIC_EnableIRQ(ADC_THCMP_IRQn);

  // blocking wait for threshold event
  // TODO add timeout
  while ( _adc_dma_trigger_offset < 0 )
  {
    cfg_dma_xfer(DMA_BUFFER_SIZE);

    // Start sampling using hardware timer
    enable_sample_timer();

    // Wait until DMA is finished
    while ( adc_dma_busy() ) { }
  }

  // Get another 1K sample if the trigger occurs
  if ( _adc_dma_trigger_offset >= 0 )
  {
    // Continue to sample another 1K in the second buffer
    LPC_ADC->INTEN = (1 << SEQA_INTEN);

    // DMA transfer config
    uint32_t xfercfg = 1 << DMA_XFERCFG_CFGVALID |
                       // 1 << DMA_XFERCFG_CLRTRIG |
                       1 << DMA_XFERCFG_SETINTA |
                       0 << DMA_XFERCFG_SETINTB |
                       1 << DMA_XFERCFG_WIDTH | 	// 16 bits for 12-bit ADC values
                       0 << DMA_XFERCFG_SRCINC | 	// TODO multiple ADC
                       1 << DMA_XFERCFG_DSTINC |
                       (DMA_BUFFER_SIZE - 1) << DMA_XFERCFG_XFERCOUNT;

    // Channel Descriptor, the first descriptor
    Chan_Desc_Table[0].source = (uint32_t) &LPC_ADC->DAT[_channel];
    Chan_Desc_Table[0].dest = (uint32_t) &adc_buffer[2*DMA_BUFFER_SIZE - 1];
    Chan_Desc_Table[0].next = 0;

    // Set the valid bit for channel 0 in the SETVALID register
    LPC_DMA->SETVALID0 = 1 << 0;

    // Set XFERCFG register, which will put DMA into ready mode
    LPC_DMA->CHANNEL[0].XFERCFG = xfercfg;

    // Start sampling using hardware timer
    enable_sample_timer();
  }
}

void adc_dma_stop(void)
{
  // If DMA is busy, wait until it is finished
  while ( adc_dma_busy() ) { }

  // Disable sampling timer
  disable_sample_timer();
}

bool adc_dma_busy(void)
{
  return LPC_DMA->CHANNEL[0].XFERCFG & (1 << DMA_XFERCFG_CFGVALID);
}

// Return the sample that cause the threshold interrupt to fire
int16_t adc_dma_get_threshold_sample(void)
{
	return _adc_dma_trigger_offset;
}


uint16_t *adc_dma_get_buffer()
{
	return adc_buffer;
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

  // End of DMA descriptor chain, disable timer ADC trigger
  if ( ! (LPC_DMA->CHANNEL[0].XFERCFG & (1 << DMA_XFERCFG_CFGVALID)) )
  {
    // Disable sampling timer
    disable_sample_timer();
  }
}

// This interrupt handler determines the sample that triggered the threshold interrupt
void ADC_THCMP_IRQHandler(void)
{
  // Only check the threshold interrupt status of our ADC channel
  uint32_t intsts = (LPC_ADC->FLAGS & (1 << _channel)) ;

  if ( intsts )
  {
    // The sample that caused the interrupt
    uint16_t offset_countdown = ((LPC_DMA->CHANNEL[0].XFERCFG & 0xFFFF0000) >> 16);
    _adc_dma_trigger_offset = (DMA_BUFFER_SIZE-1) - offset_countdown - 1;

    // Disable the threshold interrupt
    NVIC_DisableIRQ(ADC_THCMP_IRQn);
  }

  LPC_ADC->FLAGS = intsts; // clear interrupts
}
