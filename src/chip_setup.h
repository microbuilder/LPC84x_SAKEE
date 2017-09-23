#include "board.h"


//
// The following parameters need to be defined for each project's inital clock setup (used in system.c))
// 
#define FRO_FREQ_VAL 0             // 0 = 12 MHz
                                   // 1 = 24 MHz (reset value)
                                   // 2 = 30 MHz

#define FRO_LOW_POWER_START_VAL 0  // 0 = divide_by_2 (reset_value)
                                   // 1 = divide_by_16 (Only use if FAIM has been programmed for this)

#define FRO_DIRECT_VAL 1           // 0 = fro_dividers_out (reset value)
                                   // 1 = fro_oscout

#define EXTCLKSEL_VAL 1            // 0 = sys_osc_clk (reset value)
                                   // 1 = clk_in

#define SYSPLLCLKSEL_VAL 0         // 00 = fro (reset value)
                                   // 01 = external_clk
                                   // 10 = wdt_osc_clk
                                   // 11 = fro_div

#define MAINCLKSEL_VAL 0           // 00 = fro (reset value)
                                   // 01 = external_clk
                                   // 10 = wdt_osc_clk
                                   // 11 = fro_div

#define MAINCLKPLLSEL_VAL 0        // 00 = main_clk_pre_pll (reset value)
                                   // 01 = sys_pll0_clk
                                   // 10 = off
                                   // 11 = off

#define SYSOSCCTRL_VAL 0           // 00 = 1 - 20 MHz frequency range (reset value)
                                   // 10 = 15 - 25 MHz frequency range
                                   // x1 = Use external clock input source, instead of crystal oscillator

#define WDTOSCCTRL_VAL 0x000       // bits 4:0 = DIVSEL (reset value = 00000)
                                   // bits 8:5 = FREQSEL, a.k.a. Fclkana (reset value = 0000)
                                   //            <0=> 0 MHz
                                   //            <1=> 0.6 MHz
                                   //            <2=> 1.05 MHz
                                   //            <3=> 1.4 MHz
                                   //            <4=> 1.75 MHz
                                   //            <5=> 2.1 MHz
                                   //            <6=> 2.4 MHz
                                   //            <7=> 2.7 MHz
                                   //            <8=> 3.0 MHz
                                   //            <9=> 3.25 MHz
                                   //            <10=> 3.5 MHz
                                   //            <11=> 3.75 MHz
                                   //            <12=> 4.0 MHz
                                   //            <13=> 4.2 MHz
                                   //            <14=> 4.4 MHz
                                   //            <15=> 4.6 MHz                             
                                   // wdt_osc_clk = FREQSEL / (2 � (1 + DIVSEL))

#define SYSPLLCTRL_VAL 0x00        // bits 4:0 = MSEL: Feedback Divider Selection (reset value = 00000)
                                   // bits 6:5 = PSEL: Post Divider Selection (reset value = 00)
                                   //            F_clkout = M * F_clkin = F_CCO / (2 * P)
                                   //            F_clkin must be in the range of  10 MHz to  25 MHz
                                   //            F_CCO   must be in the range of 156 MHz to 320 MHz
                                   //            M = MSEL + 1
                                   //            PSEL = 00 ... P = 1
                                   //            PSEL = 01 ... P = 2
                                   //            PSEL = 10 ... P = 4
                                   //            PSEL = 11 ... P = 8

#define SYSAHBCLKDIV_VAL 1         // 0x00 = system_ahb_clk disabled (use with caution)
                                   // 0x01 = divide_by_1 (reset value)
                                   // 0x02 = divide_by_2
                                   // 0xFF = divide_by_255

#define XTAL_CLK_VAL 12000000      // System Oscillator (XTAL) Frequency [Hz] must be in the range of  1 MHz to  25 MHz

#define CLKIN_CLK_VAL 12000000     // External Clock (CLKIN) frequency [Hz] must be in the range of  1 MHz to  25 MHz            


#define EXT_CLOCK_FORCE_ENABLE 0   // Force config. and enable of external_clk for use by other than main_clk or sys_pll0_clk
                                   // 0 = external_clk will be configured and enabled only if needed by main_clk or sys_pll0_clk.
                                   // 1 = external_clk will be configured and enabled (available for other, e.g. clock out).

// End of clocks configuration section










//
// The following parameters need to be defined for projects that use the debug UART (used in serial.c)
//
#define DBGUART 0                  // Choose the index for the debug UART (0 for UART0, 1 for UART1, etc.)
#define DBGBAUDRATE 9600           // Choose the baud rate for the debug UART
#define USE_MBED_PORT 1            // '1' to use MBED serial port, '0' to use user-defined port pins for debug UART

#if (USE_MBED_PORT == 1)
  #define DBGTXPIN TARGET_TX       // For MBED serial port (see board.h)
  #define DBGRXPIN TARGET_RX       // For MBED serial port (see board.h)
#else
  #define DBGTXPIN P0_25           // Use with USB-to-RS232 break-out cable (choose your own favorite TxD pin)
  #define DBGRXPIN P0_24           // Use with USB-to-RS232 break-out cable (choose your own favorite RxD pin)
#endif

//
// The following are so the debug UART is selectable from any UART on the device (used in Serial.c)
//
#define __CONCAT(x,y,z) x##y##z
#define __XCONCAT(x,y,z) __CONCAT(x,y,z)
 
#define INDEX   DBGUART
#define pDBGU   __XCONCAT(LPC_USART,INDEX,)
#define DBGU    __XCONCAT(UART,INDEX,)
#define DBGUTXD __XCONCAT(U,INDEX,_TXD)
#define DBGURXD __XCONCAT(U,INDEX,_RXD)
#define DBGURST __XCONCAT(UART,INDEX,_RST_N)
#define DBGUIRQ __XCONCAT(UART,INDEX,_IRQn)




// CAPTOUCH
// Define the analog comparator input to use
//
#define ACMP_IN_FOR_CAPTOUCH      ACOMP_IN5
#define ACMP_I_FOR_CAPTOUCH       ACMP_I5
#define ACMP_I_PORT_FOR_CAPTOUCH  ACMP_I5_PORT

//
// Associate the CAPT IRQ slot with the ACMP IRQ slot in the startup code, since they are shared
//
#define CAPT_IRQHandler CMP_IRQHandler

//
// Define some constants to be used
//
#define FREQ 4000000           // Set the divided FCLK frequency
#define NUM_CALIB_SAMPLES 1000 // Set the number of times to sample during no touch baseline calibration
#define NM_FRACTION 1          // Noise margin = (notouch_high-notouch_low)/NM_FRACTION
#define NUM_SENSORS 2          // How many sensors / X pins are being used
#define LOW_SENSOR 0           // The index of the lowest X pin used by this software
#define CHAIN_LENGTH 64        // Number of taps of the DC finding decoder
#define TOUCH_TRIGGERS_LOWER 1 // '1' if touch triggers lower, '0' if touch triggers higher than no-touch
#define LP_MODE 2              // '0' for sleep, '1' for deep-sleep, '2' for power-down

// None of the following are applicable to CapTouch buttons examples
#define DB_VAL 75              // If using debounce, count up to this number
#define DEBOUNCE 0             // '1' to introduce a debounce delay on the first touch after a no-touch
#define FIR 0                  // '1' to use FIR averaging filter
#define IIR 0                  // '1' to use IIR averaging filter
#define BUTT 0                 // '1' to use digital Butterworth LPF
#define DECIM 0                // '1' to use decimating filter
#define NUM_SAMPLES 8          // Memory of the FIR filter (a.k.a. 'L')
#define FILTER_GAIN 3          // Log2 of Filter gain for the FIR and IIR filters. Must always = Log2 of NUM_SAMPLES
#define DECIMATE_FACTOR 4      // If using decimating filter decimate by this factor

#define PWM_FREQ 100      // in cycles/second
#define PWM_RES  100      // in counts/cycle

#define CAPT_MRT_CH       1
