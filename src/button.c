/*
===============================================================================
 Name        : button.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : GPIO button debounce helper
===============================================================================
*/

#include "LPC8xx.h"
#include "gpio.h"

#include "config.h"
#include "delay.h"
#include "button.h"

#define PORT_COMMON 0

static uint32_t _all_mask = bit(QEI_SW_PIN) | bit(BUTTON_ISP) | bit(BUTTON_WAKE) | bit(BUTTON_USER1) | bit(BUTTON_USER2);

// Returns the button read in a 32-bit bitfield
static inline uint32_t button_read(void)
{
  // button is active LOW, invert value before compare
  return (~LPC_GPIO_PORT->PIN[PORT_COMMON]) & _all_mask;
}

void button_init(void)
{
	// Initialize Button
  LPC_GPIO_PORT->DIRCLR[PORT_COMMON] = _all_mask;
}

/**
 * Check if a button has been pressed, including basic debouncing.
 *
 * Note: Press and hold will be reported as a single click, and
 *       release isn't reported in the code below. Only a single
 *       press event will register as a 1 at the appropriate bit.
 *
 * @return Bitmask of pressed buttons e.g If BUTTON_A is pressed
 * bit 31 will be set.
 */
uint32_t button_pressed(void)
{
  // must be exponent of 2
  enum { MAX_CHECKS = 2, SAMPLE_TIME = 5 };

  /* Array that maintains bounce status, which is sampled
   * at 10 ms/lsb. Debounced state is valid if all values
   * on a switch maintain the same state (bit set or clear)
   */
  static uint32_t lastReadTime = 0;
  static uint32_t states[MAX_CHECKS] = { 0 };
  static uint32_t index = 0;

  // Last debounce state, used to detect changes
  static uint32_t lastDebounced = 0;

  // Too soon, nothing to do
  if (millis() - lastReadTime < SAMPLE_TIME ) return 0;

  lastReadTime = millis();

  // Take current read and mask with BUTTONs
  // Note: Bitwise inverted since buttons are active (pressed) LOW
  uint32_t debounced = button_read();

  // Copy current state into array
  states[ (index & (MAX_CHECKS-1)) ] = debounced;
  index++;

  // Bitwise 'and' all the states in the array together to get the result
  // Pin must stay asserted at least MAX_CHECKS time to be recognized as valid
  for(int i=0; i<MAX_CHECKS; i++)
  {
    debounced &= states[i];
  }

  // 'result' = button changed and passes debounce checks, 0 = failure or not-asserted
  uint32_t result = (debounced ^ lastDebounced) & debounced;

  lastDebounced = debounced;

  return result;
}

