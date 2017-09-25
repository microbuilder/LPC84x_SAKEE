///////////////////////////////////////////////////////////////////////////////
// Functions.c for CapTouch examples
///////////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <stdio.h>
#include "LPC8xx.h"
#include "acomp.h"
#include "capt.h"
#include "utilities.h"
#include "syscon.h"
#include "swm.h"
#include "iocon.h"
#include "chip_setup.h"
#include "mrt.h"
#include "uart.h"
#include "sct.h"
#include "ctimer.h"


volatile uint32_t notouch_baseline[NUM_SENSORS];
volatile uint32_t mean_notouch_baseline;
volatile uint32_t ntch_noise_margin, touch_threshold;
volatile uint32_t ntch_high, ntch_low;
volatile uint32_t saved_low_cnt, saved_high_cnt;
volatile bool mrt_expired;
volatile bool in_lp_mode;
extern volatile uint32_t duty_cycle[NUM_SENSORS];



//
// find_larger() function
//
uint8_t find_larger(uint32_t a, uint32_t b) {
  if (a > b)
    return 1;
  return 0;  
}


//
// is_equal() function
//
bool is_equal(uint8_t a, uint8_t b) {
  if (a == b)
    return 1;
  return 0;
}


//
// find_smaller() function
//
uint8_t find_smaller(uint32_t a, uint32_t b) {
  if (a < b)
    return 1;
  return 0;  
}


//
// Clear_Duty_Cycles()
//
void Clear_Duty_Cycles(void) {
  uint8_t k;
  for (k = 0; k<NUM_SENSORS; k++) {
   duty_cycle[k] = 0;
  }
}


//
// Update_SCT_PWM_Regs()
//
void Update_SCT_PWM_Regs(void) {
  uint8_t k;
  for (k = 0; k<NUM_SENSORS; k++) {
    LPC_SCT->MATCHREL[k].U = 100-duty_cycle[k]; //
  }
}


//
// Update_CTimer_PWM_Regs()
//
void Update_CTimer_PWM_Regs(void) {
  LPC_CTIMER0->MR[0] = duty_cycle[0];
  LPC_CTIMER0->MR[1] = duty_cycle[1];
  LPC_CTIMER0->MR[2] = duty_cycle[2];
}



//
// Compute_Notouch_Baseline() function
//
// This function changes XPINSEL and POLLMODE in ctrl reg, but restores them before returning.
// It uses a software loop to delay the equivalent time as RDELAY between POLLNOWs.
//
void Compute_Notouch_Baseline(void) {
  uint32_t temp_ctrl, save_ctrl, save_poll, n, k, s, divideby, rdelay, mult, waitclks, temp_count;

  // Read and save the current control reg. and poll_tcnt reg. configurations
  save_ctrl = LPC_CAPT->CTRL;
  save_poll = LPC_CAPT->POLL_TCNT;
  
  // Calculate the number of clocks to wait in reset/draning cap. state between POLLNOWs based on current settings 
  divideby = 1 + ((save_ctrl & (0xF<<FDIV))>>FDIV);
  rdelay   = (save_poll & (0x3<<RDELAY))>>RDELAY;
  if (rdelay == 0) {                                     // RDELAY = 0 means no added delay
    mult = 0;
  }
  else {                                                 // RDELAY = 1, 2, 3 means add 1+(2^RDELAY) divided FCLKs
    mult = 1 + (1<<rdelay);
  }
  waitclks = mult*divideby;                              // Multiply by the function clock divider to calculate number of system clocks
  
  // Set the ctrl reg. for pollnow with one X pin active for baseline calc. (based on pollmode normal, where only 1 pin is active at a time)
  temp_ctrl = save_ctrl & ~((0x3<<POLLMODE));
  temp_ctrl |= (POLLMODE_NOW);
  mean_notouch_baseline = 0;
  saved_low_cnt = 0xFFF;
  saved_high_cnt = 0;
  
  // Do a bunch of POLLNOWs, separated by the reset delay, and compute the mean count-to-trigger for no-touch
  for (s=0; s!=NUM_SENSORS; s++) {
    notouch_baseline[s] = 0;
    temp_ctrl &= ~((uint32_t)0xFFFF<<XPINSEL);
    temp_ctrl |= (1<<(s+LOW_SENSOR))<<XPINSEL; // Shift this for the 3sensor slider case
    
    for (n = 0; n != NUM_CALIB_SAMPLES; n++) {
      LPC_CAPT->CTRL = temp_ctrl;                          // Do a poll-now
      while (!((LPC_CAPT->STATUS)&POLLDONE));              // Wait for POLLDONE = 1
      
      // Stuff for auto-calculating the threshold
      temp_count = (LPC_CAPT->TOUCH & TOUCH_COUNT);
      if (find_larger(temp_count, saved_high_cnt))
        saved_high_cnt = temp_count;
      if (find_smaller(temp_count, saved_low_cnt))
        saved_low_cnt = temp_count;
      
      notouch_baseline[s] += (LPC_CAPT->TOUCH & TOUCH_COUNT); // Add the current count to the running total
      if (waitclks == 0)                                   // RDELAY = 0 means no added delay
        continue;
      for (k=0; k<waitclks/7; k++);                        // Else, wait in reset for the time specified by RDELAY
    }                                                      // The "for" loop takes 7 system clocks per iteration
    
    notouch_baseline[s] /= NUM_CALIB_SAMPLES;              // Calculate the mean, this is the no-touch baseline for this sensor
    mean_notouch_baseline += notouch_baseline[s];          // Update the running sum for all the sensors
  }
  
  ntch_high = notouch_baseline[0];
  ntch_low = notouch_baseline[0];
  for (s=0; s!=NUM_SENSORS-1; s++) {
    if (find_larger(notouch_baseline[s+1], ntch_high))
      ntch_high = notouch_baseline[s+1];
    if (find_smaller(notouch_baseline[s+1], ntch_low))
      ntch_low = notouch_baseline[s+1];  
  }
  
  //ntch_noise_margin = 0;
  ntch_noise_margin = (ntch_high - ntch_low)/NM_FRACTION; 
  //ntch_noise_margin = (saved_high_cnt - saved_low_cnt)/NM_FRACTION;
#if TOUCH_TRIGGERS_LOWER == 1
  touch_threshold = ntch_low - (ntch_high - ntch_low + ntch_noise_margin);
  //touch_threshold = saved_low_cnt - (saved_high_cnt - saved_low_cnt);
#else
  touch_threshold = ntch_high + (ntch_high - ntch_low + ntch_noise_margin);
  //touch_threshold = saved_high_cnt + (saved_high_cnt - saved_low_cnt);
#endif
  
  mean_notouch_baseline /= NUM_SENSORS;                    // Calculate the average for all the sensors
  LPC_CAPT->CTRL = save_ctrl;                              // Restore CTRL reg.


  // Initialize the filters for all sensors to all 0s
  //for (k=0; k!=NUM_SENSORS; k++) {
  //  for (n=0; n!=NUM_SAMPLES; n++) {
  //    filter[k][n] = 0;
  //  }
  //}
 
  // Initialize other variables
  //debounce_cnt = 0;
  //debounce_done = 0;
  //touching = 0;
  //ovr_cnt = 0;
  //to_cnt = 0;
  
  return;
} // end of function



//
// Comparator setup function for CapTouch examples
//
void Setup_Analog_Comparator(void) {

  // Enable power to the Acomp
  LPC_SYSCON->PDRUNCFG &= ~(ACMP_PD);

  // Enable clocks to relevant peripherals
  Enable_Periph_Clock(CLK_ACMP);
  Enable_Periph_Clock(CLK_SWM);
  Enable_Periph_Clock(CLK_IOCON);

  // Give the Analog comparator module a reset
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

} // end of function




//
// Enter_LP_Mode
//
void Enter_LP_Mode() {
  uint8_t k;
  
  // Disable CAP Touch interrupts 
  LPC_CAPT->INTENCLR = YESTOUCH|NOTOUCH|TIMEOUT|OVERRUN;

  // Clear any pending interrupt flags
  LPC_CAPT->STATUS = YESTOUCH|NOTOUCH|TIMEOUT|OVERRUN;

  printf("\n\rSleeping.\n\r");
  while (!((LPC_USART0->STAT) & TXRDY)); // Wait for Tx buffer empty

  // Power-up and switch CAP Touch FCLK to wdt_osc_clk
  LPC_SYSCON->PDRUNCFG &=  ~(WDTOSC_PD);  // Power-up the WDTOSC
  LPC_SYSCON->PDSLEEPCFG &= ~(WDTOSC_PD); // WDTOSC stay powered in dsleep/pd mode
  LPC_SYSCON->WDTOSCCTRL = (0x5<<WDTOSC_FREQSEL)|(0<<WDTOSC_DIVSEL); // 2.1 MHz / 2 ~= 1.05 MHz
  LPC_SYSCON->CAPTCLKSEL = CAPTCLKSEL_WDT_OSC_CLK; // Switch CAP Touch clock to wdt_osc_clk

  // Prepare PDSLEEPCFG and PDAWAKECFG for low power mode
  LPC_SYSCON->BODCTRL &= ~(1<<BODRSTENA); // Disable BOD reset before de-powering BOD
  LPC_SYSCON->PDSLEEPCFG |= BOD_PD;       // Power down the BOD in Deep Sleep / PD modes
  LPC_SYSCON->PDSLEEPCFG &= ~WDTOSC_PD;   // Keep the WDTOSC powered in Deep Sleep / PD modes
  LPC_SYSCON->PDAWAKECFG = LPC_SYSCON->PDRUNCFG;

  // Don't clock the clock select muxes and function clock dividers that aren't needed
  LPC_SYSCON->CLKOUTSEL = CLKOUTSEL_OFF;
  LPC_SYSCON->SCTCLKSEL = SCTCLKSEL_OFF;
  LPC_SYSCON->FRG0CLKSEL = FRGCLKSEL_OFF;
  LPC_SYSCON->FRG1CLKSEL = FRGCLKSEL_OFF;
  LPC_SYSCON->ADCCLKSEL = ADCCLKSEL_OFF;
  for (k = 1; k <= 10; k++) {
    LPC_SYSCON->FCLKSEL[k] = FCLKSEL_OFF;
  }

  // Put all GPIOs as outputs driving 1
  LPC_GPIO_PORT->DIR[0] = 0xFFFFFFFF;
  LPC_GPIO_PORT->PIN[0] = 0xFFFFFFFF;
  LPC_GPIO_PORT->DIR[1] = 0xFFFFFFFF;
  LPC_GPIO_PORT->PIN[1] = 0xFFFFFFFF;

  // Turn off peripheral bus clocks
  LPC_SYSCON->SYSAHBCLKCTRL[0] &= ~(UART0|IOCON|SWM|GPIO0|GPIO1);
  
  // First set POLLMODE to 0 and change trigger type to YH port pin
  // Then add a POLL delay (POLL_TCNT reg)
  // Then restore POLLMODE to 2 (continuous)
  // Addding a poll delay. Only need to do 1 polling round every 1/10 or 1/4 second 
  // 1exp6 clocks/sec * 100exp-3 sec = 100,000 clocks ... POLL = 100,000/4096 = 24
  // 1exp6 clocks/sec * 250exp-3 sec = 250,000 clocks ... POLL = 250,000/4096 = 61

  LPC_CAPT->CTRL &= ~(0x3<<POLLMODE); // FIRST stop polling! THIS IS IMPORTANT
  LPC_CAPT->CTRL &= ~TYPE_TRIGGER_ACMP; // set trigger type = gpio
  LPC_CAPT->POLL_TCNT &= ~(0xFF<<POLL); // Clear current value of poll delay
  LPC_CAPT->POLL_TCNT |= 61<<POLL;      // put in sleeping poll delay
  LPC_CAPT->CTRL |= POLLMODE_CONTINUOUS; // Start polling 
  
  in_lp_mode = 1;
  mrt_expired = 0;

  // Enable TOUCH interrupt only
  LPC_CAPT->INTENSET = YESTOUCH;

#if LP_MODE == 1
  // Set the SleepDeep bit
  SCB->SCR |= (1<<2);
  // PCON = 1 selects Deep-sleep mode
  LPC_PMU->PCON = 0x1;            
#elif LP_MODE == 2
  // Set the SleepDeep bit
  SCB->SCR |= (1<<2);
  // PCON = 2 selects Power-down mode mode
  LPC_PMU->PCON = 0x2;
#else
  // Clear the SleepDeep bit
  SCB->SCR &= ~(1<<2);
  // PCON = 0 selects sleep mode
  LPC_PMU->PCON = 0x0;
#endif
  // Wait here for wakeup
  __WFI(); 

}



//
// Enter_Normal_Mode
//
void Enter_Normal_Mode() {
  
  // Revert CAP Touch clock to FRO
  LPC_SYSCON->CAPTCLKSEL = CAPTCLKSEL_FRO_CLK;
  
  // Restore bus clocks to necessary peripherals
  LPC_SYSCON->SYSAHBCLKCTRL[0] |= (UART0|GPIO0|GPIO1);
  
  #if 0
  // Restart the MRT and clear the mrt_expired flag on wakeup
  LPC_MRT->Channel[0].INTVAL = ForceLoad | 120000000;
  mrt_expired = 0;
  #endif

  // Revert to analog comparator measurement method and no poll delay
  LPC_CAPT->CTRL &= ~(0x3<<POLLMODE);    // Go to poll mode off
  LPC_CAPT->POLL_TCNT &= ~((0xFF<<POLL));  // No poll delay
  //LPC_CAPT->POLL_TCNT &= ~(0xFFF<<TCNT);
  //LPC_CAPT->POLL_TCNT |= touch_threshold<<TCNT; // revert thethreshold to the normal value
  
  LPC_CAPT->CTRL |= POLLMODE_CONTINUOUS|TYPE_TRIGGER_ACMP; //Revert to ACMP measurement

  in_lp_mode = 0;
  
  LPC_CAPT->INTENSET = YESTOUCH|NOTOUCH|TIMEOUT|OVERRUN;

}



//
// MRT setup function
//
void Setup_MRT(void) {
  mrt_expired = 0; //return;
  
  // Enable clock
  Enable_Periph_Clock(CLK_MRT);

  // Give a reset
  Do_Periph_Reset(RESET_MRT);
  
  // Mode = one shot, interrupt = enable
  LPC_MRT->Channel[0].CTRL = (MRT_OneShot<<MRT_MODE) | (1<<MRT_INTEN);
  
  // Enable the MRT interrupt in the NVIC
  NVIC_EnableIRQ(MRT_IRQn);
  
  // Start it
  LPC_MRT->Channel[0].INTVAL = ForceLoad | 120000000; // Five seconds at 24 MHz

}

#if 0
//
// SCT setup function for doing PWM
//
void Setup_SCT_PWM(void) {
  uint32_t count_freq, prescale;

  // Determine the prescaler (want to count from 0 to PWM_RES every 1/PWM_FREQ seconds)
  // PRE+1 = clock_freq /(pwm_freq)*(pwm_res)
  count_freq = PWM_FREQ * PWM_RES;       // in counts/second
  prescale = (main_clk/count_freq) - 1;  // in clocks/count
  
  // Enable clock
  Enable_Periph_Clock(CLK_SCT);
  Enable_Periph_Clock(CLK_SWM);

  // Assign SCT outputs to port pins for 6 LEDs
  ConfigSWM(SCT_OUT0, LED_0);
  ConfigSWM(SCT_OUT1, LED_1);
  ConfigSWM(SCT_OUT2, LED_2);
  ConfigSWM(SCT_OUT3, LED_3);
  ConfigSWM(SCT_OUT4, LED_4);
  ConfigSWM(SCT_OUT5, LED_5);

  // event 7: Match in MR7, counter limit, sets OUT0-OUT5 (LEDs off), active in state 0
  // event 5: Match in MR5, clears OUT5 (LED on), active in state 0
  // event 4: Match in MR4, clears OUT4 (LED on), active in state 0
  // event 3: Match in MR3, clears OUT3 (LED on), active in state 0
  // event 2: Match in MR2, clears OUT2 (LED on), active in state 0
  // event 1: Match in MR1, clears OUT1 (LED on), active in state 0
  // event 0: Match in MR0, clears OUT0 (LED on), active in state 0

  // UNIFY counter, CLKMODE=busclock, CKSEL=unused(default), NORELOAD=fale, INSYNC=unused(default), AUTOLIMIT=false
  LPC_SCT->CONFIG |= (1<<UNIFY) |
                     (Bus_clock<<CLKMODE) |
                     (0<<NORELOAD_L) |
                     (0<<AUTOLIMIT_L);

  // Don't run yet, clear the counter, BIDIR=0(default,unidirectional up-count), PRE=caculated prescale value
  LPC_SCT->CTRL   |= (0<<Stop_L) |       // Stay in halt mode until SCT setup is complete
                     (1<<Halt_L) |       // Stay in halt mode until SCT setup is complete
                     (1<<CLRCTR_L) |     // Clear the counter (good practice)
                     (0<<BIDIR_L) |      // Unidirectional mode (Up-count)
                     (prescale<<PRE_L);  // Prescaler 

  // Event 7 is used as the counter limit
  LPC_SCT->LIMIT_L = 1<<7;

  // No events will set the HALT_L bit in the CTRL reg.
  LPC_SCT->HALT_L = 0;

  // No events will set the STOP_L bit in the CTRL reg.
  LPC_SCT->STOP_L = 0;

  // No events will clear the STOP_L bit in the CTRL reg.
  LPC_SCT->START_L = 0;

  // Initialize the COUNT register; Start counting at '0'
  LPC_SCT->COUNT = 0;

  // Start in state 0
  LPC_SCT->STATE_L = 0;

  // All Match/Capture registers act as match registers
  LPC_SCT->REGMODE_L = 0;

  // Initialize OUT0 - OUT5 to '1' so the LEDs are off to begin with
  LPC_SCT->OUTPUT = (1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5);

  // Configure the OUTPUTDIRCTRL register
  // The counting direction has no impact on the meaning of set and clear for all outputs
  LPC_SCT->OUTPUTDIRCTRL = 0;

  // Configure the RES register
  // Simultaneous set and clear (which would be a programming error, and won't happen here) has no effect for all outputs
  LPC_SCT->RES = 0;

  // Configure the EVEN register 
  // This example does not use interrupts, so don't enable any event flags to interrupt.
  LPC_SCT->EVEN = 0;

  // Clear any pending event flags by writing '1's to the EVFLAG register
  LPC_SCT->EVFLAG = 0x3F;

  // Configure the CONEN register
  // This example does not use interrupts, so don't enable any 'no-change conflict' event flags to interrupt.
  LPC_SCT->CONEN = 0;

  // Clear any pending 'no-change conflict' event flags, and BUSSERR flags, by writing '1's to the CONFLAG register
  LPC_SCT->CONFLAG = 0xFFFFFFFF;

  // Initialize the match registers to 0% duty cycle, initialize the period
  LPC_SCT->MATCH[0].U =    100-0;
  LPC_SCT->MATCH[1].U =    100-0;
  LPC_SCT->MATCH[2].U =    100-0;
  LPC_SCT->MATCH[3].U =    100-0;
  LPC_SCT->MATCH[4].U =    100-0;
  LPC_SCT->MATCH[5].U =    100-0;
  LPC_SCT->MATCH[7].U =    PWM_RES;

  // Same for the match reload registers.
  LPC_SCT->MATCHREL[0].U =    100-0;
  LPC_SCT->MATCHREL[1].U =    100-0;
  LPC_SCT->MATCHREL[2].U =    100-0;
  LPC_SCT->MATCHREL[3].U =    100-0;
  LPC_SCT->MATCHREL[4].U =    100-0;
  LPC_SCT->MATCHREL[5].U =    100-0;
  LPC_SCT->MATCHREL[7].U =    PWM_RES;

  // Configure the EVENT_STATE and EVENT_CTRL registers for all events
  //
  // Event EVENT_STATE                        EVENT_CTRL
  // ----- ---------------------------------  -------------------------------------------------------------------------------------------
  // EV0   Enabled in State 0.                A match in Match0 is associated with this event, no effect on state.
  // EV1   Enabled in State 0.                A match in Match1 is associated with this event, no effect on state.
  // EV2   Enabled in State 0.                A match in Match2 is associated with this event, no effect on state.
  // EV3   Enabled in State 0.                A match in Match3 is associated with this event, no effect on state.
  // EV4   Enabled in State 0.                A match in Match4 is associated with this event, no effect on state.
  // EV5   Enabled in State 0.                A match in Match5 is associated with this event, no effect on state.
  // EV7   Enabled in State 0.                A match in Match7 is associated with this event, no effect on state.
  LPC_SCT->EVENT[0].STATE = 1<<0;                                                           // Event0 is enabled in state 0
  LPC_SCT->EVENT[1].STATE = 1<<0;                                                           // Event1 is enabled in state 0
  LPC_SCT->EVENT[2].STATE = 1<<0;                                                           // Event2 is enabled in state 0
  LPC_SCT->EVENT[3].STATE = 1<<0;                                                           // Event3 is enabled in state 0
  LPC_SCT->EVENT[4].STATE = 1<<0;                                                           // Event4 is enabled in state 0
  LPC_SCT->EVENT[5].STATE = 1<<0;                                                           // Event5 is enabled in state 0
  LPC_SCT->EVENT[7].STATE = 1<<0;                                                           // Event7 is enabled in state 0
  LPC_SCT->EVENT[0].CTRL  = (0<<MATCHSEL)|(Match_Only<<COMBMODE)|(0<<STATELD)|(0<<STATEV);  // Match in MAT0 associated, no state change
  LPC_SCT->EVENT[1].CTRL  = (1<<MATCHSEL)|(Match_Only<<COMBMODE)|(0<<STATELD)|(0<<STATEV);  // Match in MAT1 associated, no state change
  LPC_SCT->EVENT[2].CTRL  = (2<<MATCHSEL)|(Match_Only<<COMBMODE)|(0<<STATELD)|(0<<STATEV);  // Match in MAT2 associated, no state change
  LPC_SCT->EVENT[3].CTRL  = (3<<MATCHSEL)|(Match_Only<<COMBMODE)|(0<<STATELD)|(0<<STATEV);  // Match in MAT3 associated, no state change
  LPC_SCT->EVENT[4].CTRL  = (4<<MATCHSEL)|(Match_Only<<COMBMODE)|(0<<STATELD)|(0<<STATEV);  // Match in MAT4 associated, no state change
  LPC_SCT->EVENT[5].CTRL  = (5<<MATCHSEL)|(Match_Only<<COMBMODE)|(0<<STATELD)|(0<<STATEV);  // Match in MAT5 associated, no state change
  LPC_SCT->EVENT[7].CTRL  = (7<<MATCHSEL)|(Match_Only<<COMBMODE)|(0<<STATELD)|(0<<STATEV);  // Match in MAT7 associated, no state change

  // Configure the OUT registers for the SCT outputs
  // Event 0 clears OUT0 to '0'   (LED on)
  // Event 1 clears OUT1 to '0'   (LED on) 
  // Event 2 clears OUT2 to '0'   (LED on) 
  // Event 3 clears OUT3 to '0'   (LED on) 
  // Event 4 clears OUT4 to '0'   (LED on) 
  // Event 5 clears OUT5 to '0'   (LED on) 
  // Event 7 sets OUT0 - OUT5     (LEDs off)
  LPC_SCT->OUT[0].CLR = 1<<0;
  LPC_SCT->OUT[1].CLR = 1<<1;
  LPC_SCT->OUT[2].CLR = 1<<2;
  LPC_SCT->OUT[3].CLR = 1<<3;
  LPC_SCT->OUT[4].CLR = 1<<4;
  LPC_SCT->OUT[5].CLR = 1<<5;
  LPC_SCT->OUT[0].SET = 1<<7;
  LPC_SCT->OUT[1].SET = 1<<7;
  LPC_SCT->OUT[2].SET = 1<<7;
  LPC_SCT->OUT[3].SET = 1<<7;
  LPC_SCT->OUT[4].SET = 1<<7;
  LPC_SCT->OUT[5].SET = 1<<7;

  // FINALLY ... now let's run it. Clearing bit 2 of the CTRL register takes it out of HALT.
  LPC_SCT->CTRL_L &= ~(1<<Halt_L);

} // end of function



//
// CTimer setup function for doing PWM
//
void Setup_CTimer_PWM(void) {
  uint32_t count_freq;
  
  // Enable clock
  Enable_Periph_Clock(CLK_CTIMER0);
  
  // Set the PWM period in counts/cycle of the TC
  LPC_CTIMER0->MR[3] = PWM_RES;

  // Configure the prescaler (want to count from 0 to PWM_RES every 1/PWM_FREQ seconds)
  // PRE+1 = clock_freq /(pwm_freq)*(pwm_res)
  count_freq = PWM_FREQ * PWM_RES;             // in counts/second
  LPC_CTIMER0->PR = (main_clk/count_freq) - 1; // in clocks/count
  
  // Configure the Match Control register to reset on match in MR3
  LPC_CTIMER0->MCR = (1<<MR3R);
  
  // Put the Match 0 - 2 outputs into PWM mode
  LPC_CTIMER0->PWMC = (1<<PWMEN0)|(1<<PWMEN1)|(1<<PWMEN2);
  
  // Connect the Ctimer Match/PWM outputs to port pins
  ConfigSWM(T0_MAT0, LED_6);
  ConfigSWM(T0_MAT1, LED_7);
  ConfigSWM(T0_MAT2, LED_8);
  
  // Initial values for the 3 duty-cycles
  LPC_CTIMER0->MR[0] = 0;
  LPC_CTIMER0->MR[1] = 0;
  LPC_CTIMER0->MR[2] = 0;
  
  // Start the action
  LPC_CTIMER0->TCR = 1<<CEN;
 
}
#endif
