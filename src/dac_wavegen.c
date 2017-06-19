/*
===============================================================================
 Name        : dac_wavegen.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : DAC based waveform generator
===============================================================================
*/
#include "LPC8xx.h"
#include "syscon.h"
#include "iocon.h"
#include "swm.h"
#include "dac.h"
#include "gpio.h"
#include "config.h"
#include "dac_wavegen.h"

#include "chip_setup.h"

#define DAC1_IRQHandler PININT5_IRQHandler // DAC1 shares NVIC slot with PININT5 for LPC845

uint16_t const* _sample_data  = NULL;
uint32_t        _sample_count = 0;

void dac_wavegen_init(uint8_t dac_id)
{
  _sample_data  = NULL;
  _sample_count = 0;

  // Enable clocks to relevant peripherals
  LPC_SYSCON->SYSAHBCLKCTRL[0] |= SWM|IOCON|GPIO1|GPIO_INT;

  Enable_Periph_Clock( dac_id ? CLK_DAC1 : CLK_DAC0);

  // DAC1 is routed to speaker on LPC845 xpresso board ( by default)
  if (dac_id == 1)
  {
    // Clear the PININT5 interrupt flag that may be pending
    LPC_PIN_INT->IST = 1<<5;

    // For the audio amp on Xpresso board, enable pin as output and drive it as requested
    // Pin set to '1' to enable, '0' to disable the audio amp.
    GPIOSetDir(AUDIO_AMP_ENABLE_PORT, AUDIO_AMP_ENABLE_PIN, OUTPUT);
    GPIOSetBitValue(AUDIO_AMP_ENABLE_PORT, AUDIO_AMP_ENABLE_PIN, 1);
  }

  // Enable DACOUT on its pin
  LPC_SWM->PINENABLE0 &= ~( dac_id ? DACOUT1 : DACOUT0);

  // Configure the DACOUT pin. Inactive mode (no pullups/pulldowns), DAC function enabled
  volatile uint32_t* iocon_pin = (dac_id ? &LPC_IOCON->DACOUT1_PIN : &LPC_IOCON->DACOUT0_PIN);

  uint32_t temp = (*iocon_pin) & (IOCON_MODE_MASK) & (IOCON_DACEN_MASK);
  temp |= (0<<IOCON_MODE)|(1<<IOCON_DAC_ENABLE);
  (*iocon_pin) = temp;
}

void dac_wavegen_run(uint8_t dac_id, uint16_t const samples[], uint32_t count, uint32_t freq)
{
  _sample_data  = samples;
  _sample_count = count;

  LPC_DAC_TypeDef* lpc_adc = (dac_id ? LPC_DAC1 : LPC_DAC0);

  // Configure the 16-bit DAC counter with an initial Freq.
  // SamplesPerCycle * Freq = samples/sec
  lpc_adc->CNTVAL = (system_ahb_clk)/(count * freq) - 1;

  // Power to the DAC!
  LPC_SYSCON->PDRUNCFG &= ~(dac_id ? DAC1_PD : DAC0_PD);

  // Write to the CTRL register to start the action. Double buffering enabled, Count enabled.
  lpc_adc->CTRL = (1<<DAC_DBLBUF_ENA) | (1<<DAC_CNT_ENA);

  // Enable the DAC interrupt
  NVIC_EnableIRQ( dac_id ? DAC1_IRQn : DAC0_IRQn);
}

void dac_wavegen_stop(uint8_t dac_id)
{
  // TODO more work to do
  NVIC_DisableIRQ( dac_id ? DAC1_IRQn : DAC0_IRQn);
  Disable_Periph_Clock(dac_id ? CLK_DAC1 : CLK_DAC0);
}

void dac_irq(uint8_t dac_id)
{
  LPC_DAC_TypeDef* lpc_adc = (dac_id ? LPC_DAC1 : LPC_DAC0);

  static uint32_t idx = 0;

  if (_sample_data == NULL || _sample_count == 0 ) return;

  if (idx >= _sample_count) idx = 0;

  // ToDo: We need to shift the DAC values 6 bits for now ...
  // adjust the lookup tables to be pre-shifted instead!
  lpc_adc->CR = (_sample_data[idx++] << 6) & 0x0000FFFF;
}


void DAC0_IRQHandler(void)
{
  dac_irq(0);
}

void DAC1_IRQHandler(void)
{
  dac_irq(1);
}
