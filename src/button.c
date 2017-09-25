/*
===============================================================================
 Name        : button.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : GPIO button debounce helper
===============================================================================
*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "LPC8xx.h"
#include "chip_setup.h"
#include "gpio.h"
#include "config.h"
#include "delay.h"
#include "button.h"
#include "acomp.h"
#include "iocon.h"
#include "capt.h"

#define PORT_COMMON 0

static uint32_t _all_mask = bit(QEI_SW_PIN) | bit(BUTTON_ISP) | bit(BUTTON_WAKE) | bit(BUTTON_USER1) | bit(BUTTON_USER2);

#if BUTTON_USE_CAPTOUCH
static uint32_t _captouch_mask = bit(BUTTON_USER1) | bit(BUTTON_USER2);

void Compute_Notouch_Baseline(void);
extern volatile uint32_t to_cnt;
extern volatile uint32_t ovr_cnt;
extern volatile uint32_t x_data[NUM_SENSORS];
extern volatile uint8_t largest;
extern volatile uint32_t mean_notouch_baseline;
extern volatile bool touching;
extern volatile uint32_t last_touch_cnt[NUM_SENSORS];
extern volatile uint32_t touch_threshold;
extern volatile bool mrt_expired;

#endif


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

#if BUTTON_USE_CAPTOUCH
  // Enable power to the Acomp
  LPC_SYSCON->PDRUNCFG &= ~(ACMP_PD);

  // Enable clocks to relevant peripherals
  Enable_Periph_Clock(CLK_ACMP);
  Enable_Periph_Clock(CLK_SWM);
  Enable_Periph_Clock(CLK_IOCON);

  Do_Periph_Reset(RESET_ACMP);

  // CTRL register setup (when Vp>Vm, ACMP_OUT 0->1 ... when Vp<Vm, ACMP_OUT 1->0 with a little hysteresis thrown in)
  // Hysteresis=20 mV, INTENA=interrupt_disabled, Vm=Vladder, Vp=ACMP_I5
  LPC_CMP->CTRL = (_20mV<<HYS) | (0<<INTENA) | (V_LADDER_OUT<<COMP_VM_SEL) | (ACMP_IN_FOR_CAPTOUCH<<COMP_VP_SEL);

  // Voltage ladder setup
  // Reference = Vdd, choose select such that (1/2)*Vdd < select < (2/3.3)*Vdd, ladder enabled
  // (2.0 V)/(3.3 V) * 32 = 19d = 13h
  //LPC_CMP->LAD = (SUPPLY_VDD<<LADREF) | (0x0F<<LADSEL) | (1<<LADEN);
  LPC_CMP->LAD = (SUPPLY_VDD<<LADREF) | (0x10<<LADSEL) | (1<<LADEN);
  //LPC_CMP->LAD = (SUPPLY_VDD<<LADREF) | (0x13<<LADSEL) | (1<<LADEN);

  // Enable an ACMP_IN on its pin
  EnableFixedPinFunc(ACMP_I_FOR_CAPTOUCH);

  // Configure the ACMP_IN pin in IOCON. Inactive mode (no pullups/pulldowns)
  LPC_IOCON->ACMP_I_PORT_FOR_CAPTOUCH &= (IOCON_MODE_MASK|MODE_INACTIVE);

  Enable_Periph_Clock(CLK_CAPT);
  Do_Periph_Reset(RESET_CAPT);

  // Disable pullups and pulldowns on all the CAP Touch pins in IOCON
  LPC_IOCON->CAPTOUCH_X0_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  LPC_IOCON->CAPTOUCH_X1_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  //LPC_IOCON->CAPTOUCH_X2_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  //LPC_IOCON->CAPTOUCH_X3_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  //LPC_IOCON->CAPTOUCH_X4_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  //LPC_IOCON->CAPTOUCH_X5_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  //LPC_IOCON->CAPTOUCH_X6_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  //LPC_IOCON->CAPTOUCH_X7_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  //LPC_IOCON->CAPTOUCH_X8_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  LPC_IOCON->CAPTOUCH_YL_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);
  LPC_IOCON->CAPTOUCH_YH_PORT &= (IOCON_MODE_MASK|MODE_INACTIVE);

  // Enable the CAP Touch functions on their pins in the SWM
  // The threshold is very sensitive to how many X pins are enabled in SWM/IOCON even if not activated in XPINSEL
  EnableFixedPinFunc(CAPT_X0);    // P0.31
  EnableFixedPinFunc(CAPT_X1);    // P1.0
  //EnableFixedPinFunc(CAPT_X2);  // P1.1
  //EnableFixedPinFunc(CAPT_X3);  // P1.2
  //EnableFixedPinFunc(CAPT_X4);  // P1.3
  //EnableFixedPinFunc(CAPT_X5);  // P1.4
  //EnableFixedPinFunc(CAPT_X6);  // P1.5
  //EnableFixedPinFunc(CAPT_X7);  // P1.6
  //EnableFixedPinFunc(CAPT_X8);  // P1.7
  EnableFixedPinFunc(CAPT_YL);    // P1.8
  EnableFixedPinFunc(CAPT_YH);    // P1.9

  // Setup the FCLK for the CAP Touch block
  LPC_SYSCON->CAPTCLKSEL = CAPTCLKSEL_FRO_CLK;

  // Specify the divided FCLK freq.
  SystemCoreClockUpdate();

  uint32_t ctrl_reg_val, poll_tcnt_reg_val;
  ctrl_reg_val = ((fro_clk/FREQ)-1)<<FDIV;

  // Choose how selected X pins are controlled when not active (Other X during state 1, charge balance).
  ctrl_reg_val |= XPINUSE_HIGHZ;    // High-Z
  //ctrl_reg_val |= XPINUSE_LOW;    // Driven low

  // Select what is being used as the trigger
  //ctrl_reg_val |= TYPE_TRIGGER_YH; // Use the YH pin as input
  ctrl_reg_val |= TYPE_TRIGGER_ACMP; // Use the Analog comarator ACMP_0 as input

  // Initialize the control register except for no XPINSEL yet, no POLLMODE yet
  LPC_CAPT->CTRL = ctrl_reg_val;

  // Set some count values
  // Time out = 1<<TOUT = 2^TOUT divided FCLKs. 4096 divided FCLKs ~= 1 ms @ FCLK = 4 MHz
  poll_tcnt_reg_val = 12<<TOUT;

  // Poll counter is a 12 bit counter that counts from 0 to 4095 then wraps. POLL is the number of wraps to wait between polling rounds
  //poll_tcnt_reg_val |= ((FREQ)/(POLLS_PER_SEC*4096))<<POLL;
  poll_tcnt_reg_val |= 0<<POLL;

  // Choose a value for the TCHLOWER bit.
  // TCHLOWER = 1 means touch triggers at a lower count than no-touch.
  // TCHLOWER = 0 means touch triggers at a higher count than no-touch.
#if TOUCH_TRIGGERS_LOWER == 1
  poll_tcnt_reg_val |= 1U<<TCHLOWER;
#else
  poll_tcnt_reg_val |= 0<<TCHLOWER;
#endif

  // Choose an RDELAY. How many divided FCLKs to hold in step 0 (reset state or draining capacitance)
  // RDELAY has a marked effect on the threshold
  poll_tcnt_reg_val |= 3<<RDELAY;

  // Choose an MDELAY. How many divided FCLKs to wait in measurement mode before reading the YH pin or the comparator output.
  poll_tcnt_reg_val |= 1<<MDELAY;

  // Initialize the Poll and Measurement Counter register, except for the threshold count which will be calculated next.
  LPC_CAPT->POLL_TCNT = poll_tcnt_reg_val;

  // Calculate the no-touch baseline counts and initial threshold. Please don't touch sensors while this runs.
  Compute_Notouch_Baseline();

  poll_tcnt_reg_val |= touch_threshold<<TCNT;
  LPC_CAPT->POLL_TCNT = poll_tcnt_reg_val;

  // Select any or all available X pins to be used
  ctrl_reg_val |= (X0_ACTV)<<XPINSEL;
  ctrl_reg_val |= (X1_ACTV)<<XPINSEL;
  //ctrl_reg_val |= (X2_ACTV)<<XPINSEL;
  //ctrl_reg_val |= (X3_ACTV)<<XPINSEL;
  //ctrl_reg_val |= (X4_ACTV)<<XPINSEL;
  //ctrl_reg_val |= (X5_ACTV)<<XPINSEL;
  //ctrl_reg_val |= (X6_ACTV)<<XPINSEL;
  //ctrl_reg_val |= (X7_ACTV)<<XPINSEL;
  //ctrl_reg_val |= (X8_ACTV)<<XPINSEL;

  // Select the poll mode
  //ctrl_reg_val |= POLLMODE_NOW;     // One-time-only integration cycle with all selected X pins active simultaneously
  ctrl_reg_val |= POLLMODE_CONTINUOUS;// Cycle through the selected X pins continuously (with timing and delays as specified above)

  // Assign CAP Touch interrupt as wakeup source
  //LPC_SYSCON->STARTERP1 = CAPT_INT_WAKEUP;

  // Enable some CAP Touch interrupts
  LPC_CAPT->INTENSET = YESTOUCH|NOTOUCH|TIMEOUT|OVERRUN;

  // Enable the CAP Touch IRQ in the NVIC
  NVIC_EnableIRQ(CAPT_IRQn);

  // Start the action
  LPC_CAPT->CTRL = ctrl_reg_val;

  // Start the MRT.
  // The touch ISR feeds it and clears the mrt_expired flag.
  // When MRT times out, the MRT ISR sets mrt_expired and sets up the CAP Touch, et. al. for low power mode.
  // Touch interrupt wakes up into the touch ISR where CAP Touch, et. al. are reconfigured for normal polling
  // and the process repeats.
//  Setup_MRT();

#if 0
  printf("Touch a sensor\r\n\n");
  printf("BUTT   X0    X1   TOs   OVRs  TCNT\n\r");
  printf("----  ----  ----  ----  ----  ----\n\r");

	while (1)
	{
    if (touching) {
      //LPC_GPIO_PORT->CLR[0] = 1<<(largest + 15); // One sensor board LED on
      //LPC_GPIO_PORT->SET[0] = ~(1<<(largest + 15)); // All others off
      printf("%4d  ", largest);
    }
    else {
      //LPC_GPIO_PORT->SET[0] = (0x1FF<<15); // All sensor board LEDs off
      printf("      ");
    }
    for (uint32_t x=0; x<NUM_SENSORS-1; x++) {
      printf("%4d  ", x_data[x]&0xFFF);
      //printf("%4d  ", last_touch_cnt[x]&0xFFF);
    }
    printf("%4d  %4d  %4d  %4d\r", x_data[NUM_SENSORS-1]&0xFFF, to_cnt, ovr_cnt, (LPC_CAPT->POLL_TCNT>>TCNT)&0xFFF);
	}
#endif
#endif
}

uint32_t capt_pressed(void)
{
  if (!touching) return 0;

  return ( 1 << (CAPT_PAD_0+largest) );
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
  // MAX_CHECKS must be exponent of 2
  enum { MAX_CHECKS = 2, SAMPLE_TIME = 3 };

  /* Array that maintains bounce status, which is sampled
   * at 10 ms/lsb. Debounced state is valid if all values
   * on a switch maintain the same state (bit set or clear)
   */
  static uint32_t lastReadTime = 0;
  static uint32_t states[MAX_CHECKS] = { 0 };
  static uint32_t index = 0;

  // Last debounce state, used to detect changes
  static uint32_t lastDebounced = 0;

#if BUTTON_USE_CAPTOUCH
  // result always take in captouch
  uint32_t result = capt_pressed();
#else
  uint32_t result = 0;
#endif

  // Too soon, nothing to do
  if (millis() - lastReadTime < SAMPLE_TIME ) return result;

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
  result |= ((debounced ^ lastDebounced) & debounced);

  lastDebounced = debounced;

  return result;
}
