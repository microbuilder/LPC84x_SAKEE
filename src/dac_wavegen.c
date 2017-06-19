/**************************************************************************/
/*!
    @file     dac_wavegen.c
    @author   hathach

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2017, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include "LPC8xx.h"
#include "syscon.h"
#include "iocon.h"
#include "swm.h"
#include "dac.h"
#include "gpio.h"
#include "dac_wavegen.h"

#include "chip_setup.h"

// TODO temporarily
#define EN_AUDIO 1                         // '1' to enable, '0' to disable the audio amp.
#define DAC1_IRQHandler PININT5_IRQHandler // DAC1 shares NVIC slot with PININT5 for LPC845

#define SamplesPerCycle (sizeof(waveform)/4)

uint16_t const* _sample_data  = NULL;
uint32_t        _sample_count = 0;

void dac_wavegen_init(void)
{
  _sample_data  = NULL;
  _sample_count = 0;

  // Enable clocks to relevant peripherals
  LPC_SYSCON->SYSAHBCLKCTRL[0] |= SWM|IOCON|GPIO1|GPIO_INT;
  Enable_Periph_Clock(CLK_DACn);

  // Do the following only for DAC1
#if (DACn == 1)
  // Clear the PININT5 interrupt flag that may be pending and shut off clock to GPIO interrupts
  LPC_PIN_INT->IST = 1<<5;
  LPC_SYSCON->SYSAHBCLKCTRL[0] &= ~(GPIO_INT);
  // For the audio amp on Xpresso board, enable pin as output and drive it as requested
#ifdef XPRESSO_845_BOARD
  GPIOSetDir(AUDIO_AMP_ENABLE_PORT, AUDIO_AMP_ENABLE_PIN, OUTPUT);
  GPIOSetBitValue(AUDIO_AMP_ENABLE_PORT, AUDIO_AMP_ENABLE_PIN, EN_AUDIO);
#endif

#endif

  // Enable DACOUT on its pin
  LPC_SWM->PINENABLE0 &= ~(DACOUTn);

  // Configure the DACOUT pin. Inactive mode (no pullups/pulldowns), DAC function enabled
  uint32_t temp = (LPC_IOCON->DACOUTn_PIN) & (IOCON_MODE_MASK) & (IOCON_DACEN_MASK);
  temp |= (0<<IOCON_MODE)|(1<<IOCON_DAC_ENABLE);
  LPC_IOCON->DACOUTn_PIN = temp;
}

void dac_wavegen_run(uint16_t const samples[], uint32_t count, uint32_t freq)
{
  _sample_data  = samples;
  _sample_count = count;

  // Configure the 16-bit DAC counter with an initial Freq.
  // SamplesPerCycle * Freq = samples/sec
  LPC_DACn->CNTVAL = (system_ahb_clk)/(count * freq) - 1;

  // Power to the DAC!
  LPC_SYSCON->PDRUNCFG &= ~(DACn_PD);

  // Write to the CTRL register to start the action. Double buffering enabled, Count enabled.
  LPC_DACn->CTRL = (1<<DAC_DBLBUF_ENA) | (1<<DAC_CNT_ENA);

  // Enable the DAC interrupt
  NVIC_EnableIRQ(DACn_IRQn);
}

void dac_wavegen_stop(void)
{
  // TODO more work to do
  NVIC_DisableIRQ(DACn_IRQn);
}

// DAC interrupt service routine
void DACn_IRQHandler(void)
{
  static uint32_t idx = 0;

  if (_sample_data == NULL || _sample_count == 0 ) return;

  if (idx >= _sample_count) idx = 0;

  LPC_DACn->CR = _sample_data[idx++];
}

