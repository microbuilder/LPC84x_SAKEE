#include "board.h"


//
// The following parameters need to be defined for each project's inital clock setup (used in system.c))
// 
#define FRO_FREQ_VAL 2             // 0 = 18 MHz
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
                                   // wdt_osc_clk = FREQSEL / (2 × (1 + DIVSEL))

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
  #define DBGTXPIN P0_4            // Use with USB-to-RS232 break-out cable (choose your own favorite TxD pin)
  #define DBGRXPIN P0_0            // Use with USB-to-RS232 break-out cable (choose your own favorite RxD pin)
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

//
// Ditto for the DAC
//
#define DACn            1                               // '0' for DAC0, '1' for DAC1 (amazing as that may seem)
#define DACn_IRQn       __XCONCAT(DAC,DACn,_IRQn)       // expands to DAC0_IRQn, DAC1_IRQn, etc.
#define DACn_IRQHandler __XCONCAT(DAC,DACn,_IRQHandler) // expands to DAC0_IRQHandler, DAC1_IRQHandler, etc.
#define LPC_DACn        __XCONCAT(LPC_DAC,DACn,)        // expands to LPC_DAC0, LPC_DAC1, etc. 
#define DACnum          __XCONCAT(DAC,DACn,)            // expands to DAC0, DAC1, etc.
#define DACOUTn         __XCONCAT(DACOUT,DACn,)         // expands to DACOUT0, DACOUT1, etc.
#define DACOUTn_PIN     __XCONCAT(DACOUT,DACn,_PIN)     // expands to DACOUT0_PIN, DACOUT1_PIN, etc.
#define DACn_PD         __XCONCAT(DAC,DACn,_PD)         // expands to DAC0_PD, DAC1_PD, etc.
#define CLK_DACn        __XCONCAT(CLK_DAC,DACn,)        // expands to CLK_DAC0, CLK_DAC1, etc.

