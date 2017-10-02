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

#if !QEI_USE_SCT
volatile int32_t _qei_step = 0;
volatile uint8_t _a_last = 0; // pin A bit0, pin B bit1

#else

extern volatile int32_t _qei_step;

#endif

static volatile int32_t _qei_last_value = 0;

uint8_t qei_read_a(void)
{
  return bit_test(LPC_GPIO_PORT->PIN[PIN_A_PORT], PIN_A_BIT);
}

int32_t qei_abs_step (void)
{
#if QEI_USE_SCT
  return _qei_step/2;
#else
  return _qei_step;
#endif
}

// Sets the step counter to a specific value to invalidate clicks in certain situations
void qei_reset_step_val (int32_t value)
{
#if QEI_USE_SCT
  _qei_step = 2*value;
#else
  _qei_step = value;
#endif
}

int32_t qei_offset_step (void)
{
  int32_t ret = qei_abs_step() - _qei_last_value;

  _qei_last_value =  qei_abs_step();

  return ret;
}

void qei_reset_step (void)
{
  _qei_step = 0;
  _qei_last_value = 0;
}

#if !QEI_USE_SCT
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
//  LPC_SYSCON->PINTSEL[1] = QEI_B_PIN;

  // Configure the Pin interrupt mode register (a.k.a ISEL) for edge-sensitive on PINTSEL1,0
  LPC_PIN_INT->ISEL = 0x00;

  // Configure the IENR (pin interrupt enable rising) for rising edges
  LPC_PIN_INT->SIENR = 0x01;

  // Configure the IENF (pin interrupt enable falling) for falling edges
  LPC_PIN_INT->SIENF = 0x01;

  // Clear any pending or left-over interrupt flags
  LPC_PIN_INT->IST = 0xFF;

  // Get initial state
  _a_last = qei_read_a();

  NVIC_EnableIRQ(PININT0_IRQn);
//  NVIC_EnableIRQ(PININT1_IRQn);
}

/**
 * Reference http://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/
 * We can notice that the two output signals are displaced at 90 degrees out of phase from each other.
 * If the encoder is rotating clockwise the output A will be ahead of output B.So if we count the steps
 * each time the signal changes, from High to Low or from Low to High, we can notice at that time
 * the two output signals have opposite values. Vice versa, if the encoder is rotating counter clockwise,
 * the output signals have equal values. So considering this, we can easily program our controller to read
 * the encoder position and the rotation direction.
 * @param pin
 */
void qei_isr(uint8_t pin)
{
  LPC_PIN_INT->IST = bit(pin);

  uint8_t a_value = qei_read_a();

  if ( a_value != _a_last )
  {
     if ( a_value != bit_test(LPC_GPIO_PORT->PIN[PIN_B_PORT], PIN_B_BIT) )
     {
       _qei_step++;
     }else
     {
       _qei_step--;
     }

     _a_last = a_value;
  }
}

void PININT0_IRQHandler(void)
{
  qei_isr(0);
}

//void PININT1_IRQHandler(void)
//{
//  qei_isr(1);
//}
#endif
