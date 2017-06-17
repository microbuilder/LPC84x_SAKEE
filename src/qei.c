/*
===============================================================================
 Name        : quadrature.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : GPIO interrupt based quadrature decoder
===============================================================================
*/

#include "syscon.h"
#include "iocon.h"

#include "qei.h"

#define PIN_A_PORT   (QEI_A_PIN / 32)
#define PIN_A_BIT    (QEI_A_PIN % 32)

#define PIN_B_PORT   (QEI_B_PIN / 32)
#define PIN_B_BIT    (QEI_B_PIN % 32)

volatile int32_t _qei_step = 0;
volatile uint8_t enc_prev_pos = 0; // pin A bit0, pin B bit1

uint8_t qei_read_pin(void)
{
  uint8_t ret;

  // Encoder is active LOW
  ret  = bit_is_clear(LPC_GPIO_PORT->PIN[PIN_A_PORT], PIN_A_BIT) ? 0x01 : 0;
  ret |= bit_is_clear(LPC_GPIO_PORT->PIN[PIN_B_PORT], PIN_B_BIT) ? 0x02 : 0;

  return ret;
}

int32_t qei_abs_step (void)
{
  return _qei_step;
}

int32_t qei_offset_step (void)
{
  static int32_t last_value = 0;

  int32_t ret = _qei_step - last_value;

  last_value =  _qei_step;

  return ret;
}

void qei_reset_step (void)
{
  _qei_step = 0;
}

void qei_init(void)
{
  LPC_SYSCON->SYSAHBCLKCTRL0 |= (GPIO_INT);
  LPC_SYSCON->PRESETCTRL0 &= (GPIOINT_RST_N);
  LPC_SYSCON->PRESETCTRL0 |= ~(GPIOINT_RST_N);

  // Enable internal pull up. Update this if QEI PIN A & B is changed
  LPC_IOCON->PIO0_20 |= MODE_PULLUP;
  LPC_IOCON->PIO0_21 |= MODE_PULLUP;

  // Set pin direction to input
  LPC_GPIO_PORT->DIRCLR[PIN_A_PORT] = bit(PIN_A_BIT);
  LPC_GPIO_PORT->DIRCLR[PIN_B_PORT] = bit(PIN_B_BIT);

  // Configure Pin A & B as interrupt
  LPC_SYSCON->PINTSEL[0] = QEI_A_PIN;
  LPC_SYSCON->PINTSEL[1] = QEI_B_PIN;

  // Configure the Pin interrupt mode register (a.k.a ISEL) for edge-sensitive on PINTSEL1,0
  LPC_PIN_INT->ISEL = 0x00;

  // Configure the IENR (pin interrupt enable rising) for rising edges on PINTSEL0,1
  LPC_PIN_INT->SIENR = 0x3;

  // Configure the IENF (pin interrupt enable falling) for falling edges on PINTSEL0,1
  LPC_PIN_INT->SIENF = 0x3;

  // Clear any pending or left-over interrupt flags
  LPC_PIN_INT->IST = 0xFF;

  // Get initial state
  enc_prev_pos = qei_read_pin();

  NVIC_EnableIRQ(PININT0_IRQn);
  NVIC_EnableIRQ(PININT1_IRQn);
}

/**************************************************************************/
/*!
    @author   Mike Barela for Adafruit Industries
    @license  MIT

    This is an example of using the Adafruit Pro Trinket with a rotary
    encoder as a USB HID Device.  Turning the knob controls the sound on
    a multimedia computer, pressing the knob mutes/unmutes the sound

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section  HISTORY

    v1.0  - First release 1/26/2015  Mike Barela based on code by Frank Zhou
*/
/**************************************************************************/
void qei_isr(uint8_t pin)
{
  LPC_PIN_INT->IST = bit(pin);

  static uint8_t enc_flags = 0;

  uint8_t enc_cur_pos = qei_read_pin();

  // if any rotation at all
  if (enc_cur_pos != enc_prev_pos)
  {
    if (enc_prev_pos == 0x00)
    {
      // this is the first edge
      if (enc_cur_pos == 0x01) {
        enc_flags |= (1 << 0);
      }
      else if (enc_cur_pos == 0x02) {
        enc_flags |= (1 << 1);
      }
    }

    if (enc_cur_pos == 0x03)
    {
      // this is when the encoder is in the middle of a "step"
      enc_flags |= (1 << 4);
    }
    else if (enc_cur_pos == 0x00)
    {
      // this is the final edge
      if (enc_prev_pos == 0x02) {
        enc_flags |= (1 << 2);
      }
      else if (enc_prev_pos == 0x01) {
        enc_flags |= (1 << 3);
      }

      // check the first and last edge
      // or maybe one edge is missing, if missing then require the middle state
      // this will reject bounces and false movements
      if (bit_is_set(enc_flags, 0) && (bit_is_set(enc_flags, 2) || bit_is_set(enc_flags, 4))) {
        _qei_step++;
      }
      else if (bit_is_set(enc_flags, 2) && (bit_is_set(enc_flags, 0) || bit_is_set(enc_flags, 4))) {
        _qei_step++;
      }
      else if (bit_is_set(enc_flags, 1) && (bit_is_set(enc_flags, 3) || bit_is_set(enc_flags, 4))) {
        _qei_step--;
      }
      else if (bit_is_set(enc_flags, 3) && (bit_is_set(enc_flags, 1) || bit_is_set(enc_flags, 4))) {
        _qei_step--;
      }

      enc_flags = 0; // reset for next time
    }
  }

  enc_prev_pos = enc_cur_pos;
}

void PININT0_IRQHandler(void)
{
  qei_isr(0);
}

void PININT1_IRQHandler(void)
{
  qei_isr(1);
}
