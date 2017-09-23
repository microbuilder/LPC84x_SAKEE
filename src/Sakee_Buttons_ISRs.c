///////////////////////////////////////////////////////////////////////////////
// ISRs.c
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


volatile uint32_t raw_data;
volatile uint32_t temp_status;
volatile uint32_t ovr_cnt;
volatile uint32_t to_cnt;
volatile uint32_t x_data[NUM_SENSORS];
volatile uint32_t last_touch_cnt[NUM_SENSORS];
volatile uint8_t largest;
volatile bool touching;
volatile uint8_t latest_largests[CHAIN_LENGTH];
volatile uint32_t duty_cycle[NUM_SENSORS];
volatile uint32_t filter[NUM_SENSORS][NUM_SAMPLES];
volatile uint8_t current_x;
extern volatile uint32_t touch_threshold;
volatile uint32_t decim_counter[NUM_SENSORS] = {0};
extern volatile bool mrt_expired;
extern volatile bool in_lp_mode;




// Filtering function declarations
uint32_t FIR_average(uint32_t new_data);
void IIR_average(uint32_t new_data);
uint32_t Butterworth_lpf(uint32_t new_data);
uint32_t Decimator(uint32_t new_data);

// Other function declarations
uint8_t find_larger(uint32_t a, uint32_t b);
bool is_equal(uint8_t a, uint8_t b);
uint8_t find_smaller(uint32_t a, uint32_t b);
void Enter_Normal_Mode(void);




//
// CAP Touch interrupt service routine
//
void CAPT_IRQHandler(void) {
  uint32_t n;
  bool dc_flag;
  bool false_notouch;
  static uint32_t index = 0;
  
  temp_status = LPC_CAPT->STATUS;                          // Read the status flags from the STATUS register
  raw_data = LPC_CAPT->TOUCH;                              // Read the data from the TOUCH register

  // Find the sensor Xn which caused this interrupt. Load the count data to display.
  for (n=0; n!=NUM_SENSORS; n++) {
    if (((raw_data & TOUCH_XVAL)>>12) == (uint32_t)n) {  
      current_x = n;
      x_data[n] = raw_data;
      break;
    }
  }

  // On every YESTOUCH or NOTOUCH interrupt 
  if (temp_status & (YESTOUCH|NOTOUCH)) {

    // If this is a wakeup interrupt due to TOUCH, just revert to normal mode and return
    if (temp_status & (YESTOUCH)) {
      if (in_lp_mode) {
        Enter_Normal_Mode();
        printf("\n\rAwake.\n\r");
        LPC_CAPT->STATUS = YESTOUCH;              // Clear the interrupt flag by writing '1' to it
        return;                                   // Return from interrupt
      }
    }

    // Define 'false_notouch' as a NOTOUCH interrupt for any given sensor, while ANY OTHER 
    // sensor is inside of the threshold.
    // This can happen if the physically touched sensor's count actually goes outside of  
    // threshold and triggers a NOTOUCH interrupt instead of a TOUCH.
    false_notouch = 0;
    if (temp_status & (NOTOUCH)) {
      for (n=0; n<NUM_SENSORS; n++) {
        if ((x_data[n]&0xFFF) < touch_threshold) {
          false_notouch = 1;
          break;
        }
      }
    }

    if ((temp_status & (YESTOUCH)) | false_notouch) {

      // Apply your filter of choice to the new sample from this sensor Xn
#if FIR == 1
      // Call FIR L-point average filter
      last_touch_cnt[current_x] = FIR_average(raw_data & 0xFFF);
#elif IIR == 1
      // Call IIR L-point average filter
      IIR_average(raw_data & 0xFFF);
      last_touch_cnt[current_x] = filter[current_x][0];
#elif BUTT == 1
      // Call digital Butterworth low-pass filter 
      last_touch_cnt[current_x] = Butterworth_lpf(raw_data & 0xFFF);
#elif DECIM == 1
      // Call the decimating filter (envelope follower)
      n = Decimator(raw_data & 0xFFF);
      if (decim_counter[current_x] == DECIMATE_FACTOR) {
        decim_counter[current_x] = 0;
        last_touch_cnt[current_x] = n;
      }
#else
      // Update the last_touch_cnt variable for this sensor with unfiltered data
      last_touch_cnt[current_x] = raw_data & 0xFFF;    
#endif

#if DECIM == 1
  if (decim_counter[current_x] == 0) {
#endif

      // Find the sensor with the largest count of the last_touch_cnts
      largest = 0;
      for (n=0; n!=NUM_SENSORS-1; n++) {
        if (find_larger(last_touch_cnt[n+1], last_touch_cnt[largest]))
          largest = n+1;
      }
      
      // Replace the oldest of the latest_largests 
      latest_largests[index++] = largest;
      if (index == CHAIN_LENGTH)
        index = 0; 
    
      // If all elements of latest_largests[] match, report this as DC, else return from interrupt
      for (n=0; n!=CHAIN_LENGTH-1; n++) {
        if (is_equal(latest_largests[n], latest_largests[n+1])) {
          dc_flag = 1;
        }
        else {
          dc_flag = 0;
          break;
        }
      }
      touching = dc_flag;

    
#if DECIM == 1
  }
#endif

      // 'Feed' the MRT on every touch or false_notouch
      LPC_MRT->Channel[0].INTVAL = ForceLoad | 120000000;
  
      LPC_CAPT->STATUS = YESTOUCH|NOTOUCH;      // Clear the interrupt flag by writing '1' to it
      return;                                   // Return from interrupt
    }// end if ((temp_status & (YESTOUCH)) | false_notouch))
    else {                                     // This is a true NOTOUCH, all sensors are outside of threshold, revert to NOTOUCH idle
      touching = 0;
      largest = 0xFF;
      for (n=0; n!=CHAIN_LENGTH; n++) {         // Invalidate the delay chain
        latest_largests[n] = n;
      }
      LPC_CAPT->STATUS = NOTOUCH;
      return;
    } // end if((temp_status & YESTOUCH) | false_notouch)
  } // end if ((temp_status & (YESTOUCH|NOTOUCH))
  

  // Other interrupts, increment their counters, clear flags, and return
  if (temp_status & (TIMEOUT)) {
    to_cnt++;
    LPC_CAPT->STATUS = TIMEOUT;
  }
  if (temp_status & (OVERRUN)) {
    ovr_cnt++;
    LPC_CAPT->STATUS = OVERRUN;
  }
  return;
} // end of ISR



#if 0
//
// MRT ISR
//
void MRT_IRQHandler(void) {

  // Clear the interrupt flag
  LPC_MRT->Channel[0].STAT = 1<<MRT_INTFLAG;

  // Set handshake flag for main
  mrt_expired = 1;
} // end of ISR

#endif

