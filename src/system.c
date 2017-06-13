/**************************************************************************//**
 * @file     system.c
 * @brief    CMSIS Device System Source File for
 *           NXP LPC84x Device Series
 * @version  V1.0
 * @date     01. January 2017
 *
  ******************************************************************************/

//----------------------------------------------------------------------------
// Important!
// Please configure the desired initial clock setup for your project in:
// $proj_name/inc/chip_setup.h
//----------------------------------------------------------------------------


#include <stdint.h>
#include "LPC8xx.h"
#include "swm.h"
#include "syscon.h"
#include "iocon.h"
#include "fro.h"
#include "chip_setup.h"


//----------------------------------------------------------------------------
// Validate the the user's selctions
//----------------------------------------------------------------------------
#define CHECK_RANGE(val, min, max)                ((val < min) || (val > max))
#define CHECK_RSVD(val, mask)                     (val & mask)

#if (CHECK_RANGE((FRO_FREQ_VAL), 0, 2))
   #error "FRO_FREQ_VAL: Value out of range."
#endif

#if (CHECK_RANGE((FRO_LOW_POWER_START_VAL), 0, 1))
   #error "FRO_LOW_POWER_START_VAL: Value out of range."
#endif

#if (CHECK_RANGE((FRO_DIRECT_VAL), 0, 1))
   #error "FRO_DIRECT_VAL: Value out of range."
#endif

#if (CHECK_RANGE((EXTCLKSEL_VAL), 0, 1))
   #error "EXTCLKSEL_VAL: Value out of range."
#endif

#if (CHECK_RANGE((SYSPLLCLKSEL_VAL), 0, 3))
   #error "SYSPLLCLKSEL: Value out of range!"
#endif

#if (CHECK_RSVD((MAINCLKSEL_VAL),  ~0x00000003))
   #error "MAINCLKSEL: Invalid values of reserved bits!"
#endif

#if (CHECK_RSVD((MAINCLKPLLSEL_VAL),  ~0x00000003))
   #error "MAINCLKPLLSEL: Invalid values of reserved bits!"
#endif

#if (CHECK_RSVD((SYSOSCCTRL_VAL),  ~0x00000003))
   #error "SYSOSCCTRL: Invalid values of reserved bits!"
#endif

#if (CHECK_RSVD((WDTOSCCTRL_VAL),  ~0x000001FF))
   #error "WDTOSCCTRL: Invalid values of reserved bits!"
#endif

#if (CHECK_RSVD((SYSPLLCTRL_VAL),  ~0x0000007F))
   #error "SYSPLLCTRL: Invalid values of reserved bits!"
#endif

#if (CHECK_RANGE((SYSAHBCLKDIV_VAL), 0, 255))
   #error "SYSAHBCLKDIV: Value out of range!"
#endif

#if (CHECK_RANGE(XTAL_CLK_VAL, 1000000, 25000000))
   #error "XTAL frequency is out of bounds"
#endif

#if (CHECK_RANGE(CLKIN_CLK_VAL, 1000000, 25000000))
   #error "CLKIN frequency is out of bounds"
#endif



//----------------------------------------------------------------------------
//  Calculate internal clock node frequency initial values
//----------------------------------------------------------------------------
// determine output of the FRO_CLKDIV subsystem
#if FRO_FREQ_VAL == 0
  #define __FRO_OSCOUT (18000000)
#elif FRO_FREQ_VAL == 2
  #define __FRO_OSCOUT (30000000)
#else
  #define __FRO_OSCOUT (24000000)
#endif

#if FRO_LOW_POWER_START_VAL == 0
  #define __FRO_DIVIDERS_OUT (__FRO_OSCOUT / 2)
#else
  #define __FRO_DIVIDERS_OUT (__FRO_OSCOUT / 16)
#endif

#if FRO_DIRECT_VAL == 0
  #define __FRO_CLK __FRO_DIVIDERS_OUT
#else
  #define __FRO_CLK __FRO_OSCOUT
#endif

#define  __FRO_DIV_CLK (__FRO_CLK / 2)

// determine external_clk
#define __SYS_OSC_CLK  (XTAL_CLK_VAL)
#define __CLKIN_CLK    (CLKIN_CLK_VAL)
#if EXTCLKSEL_VAL == 0
  #define __EXTERNAL_CLK  __SYS_OSC_CLK
#else
  #define __EXTERNAL_CLK  __CLKIN_CLK
#endif

// determine wdt_osc_clk
#define __FREQSEL   ((WDTOSCCTRL_VAL >> 5) & 0x0F)
#define __DIVSEL   (((WDTOSCCTRL_VAL & 0x1F) + 1) << 1)
#if  (__FREQSEL ==  0)
  #define  __WDT_OSC_CLK         (0)
#elif (__FREQSEL ==  1)
  #define __WDT_OSC_CLK          ( 600000 / __DIVSEL)
#elif (__FREQSEL ==  2)
  #define __WDT_OSC_CLK          (1050000 / __DIVSEL)
#elif (__FREQSEL ==  3)
  #define __WDT_OSC_CLK          (1400000 / __DIVSEL)
#elif (__FREQSEL ==  4)
  #define __WDT_OSC_CLK          (1750000 / __DIVSEL)
#elif (__FREQSEL ==  5)
  #define __WDT_OSC_CLK          (2100000 / __DIVSEL)
#elif (__FREQSEL ==  6)
  #define __WDT_OSC_CLK          (2400000 / __DIVSEL)
#elif (__FREQSEL ==  7)
  #define __WDT_OSC_CLK          (2700000 / __DIVSEL)
#elif (__FREQSEL ==  8)
  #define __WDT_OSC_CLK          (3000000 / __DIVSEL)
#elif (__FREQSEL ==  9)
  #define __WDT_OSC_CLK          (3250000 / __DIVSEL)
#elif (__FREQSEL == 10)
  #define __WDT_OSC_CLK          (3500000 / __DIVSEL)
#elif (__FREQSEL == 11)
  #define __WDT_OSC_CLK          (3750000 / __DIVSEL)
#elif (__FREQSEL == 12)
  #define __WDT_OSC_CLK          (4000000 / __DIVSEL)
#elif (__FREQSEL == 13)
  #define __WDT_OSC_CLK          (4200000 / __DIVSEL)
#elif (__FREQSEL == 14)
  #define __WDT_OSC_CLK          (4400000 / __DIVSEL)
#else
  #define __WDT_OSC_CLK          (4600000 / __DIVSEL)
#endif

// determine main_clk_pre_pll
#if MAINCLKSEL_VAL == 0
  #define __MAIN_CLK_PRE_PLL     __FRO_CLK
#elif MAINCLKSEL_VAL == 1
  #define __MAIN_CLK_PRE_PLL     __EXTERNAL_CLK
#elif MAINCLKSEL_VAL == 2
  #define __MAIN_CLK_PRE_PLL     __WDT_OSC_CLK
#else
  #define __MAIN_CLK_PRE_PLL     __FRO_DIV_CLK
#endif

// determine sys_pll0_clk_src_i (sys_pll_clkin)
#if SYSPLLCLKSEL_VAL == 0
  #define __SYS_PLL0_CLK_SRC     __FRO_CLK
#elif SYSPLLCLKSEL_VAL == 1
  #define __SYS_PLL0_CLK_SRC     __EXTERNAL_CLK
#elif SYSPLLCLKSEL_VAL == 2
  #define __SYS_PLL0_CLK_SRC     __WDT_OSC_CLK
#else
  #define __SYS_PLL0_CLK_SRC     __FRO_DIV_CLK
#endif

// determine sys_pll0_clk (sys_pllclkout)
#define  __SYS_PLL0_CLK         (__SYS_PLL0_CLK_SRC * ((SYSPLLCTRL_VAL & 0x1F) + 1))

// determine main_clk
#if MAINCLKPLLSEL_VAL == 0
  #define __MAIN_CLK            __MAIN_CLK_PRE_PLL
#else
  #define __MAIN_CLK            __SYS_PLL0_CLK
#endif

// determine system_ahb_clk
#if USE_ROM_PLL_API == 1
  #define __SYSTEM_AHB_CLK     PLL_API_FREQ_VAL
#else
  #define __SYSTEM_AHB_CLK     (__MAIN_CLK / SYSAHBCLKDIV_VAL)
#endif



//----------------------------------------------------------------------------
// Function name: SystemInit
// Sets up the initial chip clocking based on MACROs defined in chip_setup.h.
//----------------------------------------------------------------------------
void SystemInit (void) {

  uint32_t i, temp;

  for (i = 1; i < 1; i++) __NOP();                    // To avoid a warning if variable i is unused

  // Enable clocks to IOCON and SWM upon entry, should disable them upon exit
  LPC_SYSCON->SYSAHBCLKCTRL[0] |= (SWM | IOCON);

  // Step 0. Configure the FRO subsystem (choose the source for clocks fro and fro_div)
  temp = LPC_SYSCON->FROOSCCTRL;                      // Get the current register contents
  temp &= ~((1 << FRO_DIRECT)|(FRO_FREQSEL_MASK));    // Preserve all but the bits of interest [17, 1:0] 0xfffdfffc
  #if (FRO_FREQ_VAL == 0)
    temp |= (FRO_18MHZ << FRO_FREQ_SEL);
  #elif (FRO_FREQ_VAL == 2)
    temp |= (FRO_30MHZ << FRO_FREQ_SEL);
  #else
    temp |= (FRO_24MHZ << FRO_FREQ_SEL);
  #endif
  #if (FRO_DIRECT_VAL == 1)
    temp |= (1 << FRO_DIRECT);
  #endif
  LPC_SYSCON->FROOSCCTRL = temp;                      // Update the actual register
  LPC_SYSCON->FRODIRECTCLKUEN = 0;                    // Toggle the update register for the output mux
  LPC_SYSCON->FRODIRECTCLKUEN = 1;
  while (!(LPC_SYSCON->FRODIRECTCLKUEN & 1)) __NOP(); // Wait for update to take effect
    
  // Configure sys_osc_clk or clk_in, if external_clk is needed for main_clk or sys_pll0_clk. 
  // Or, configure it if needed for other, e.g. clock out. 
  #if ((MAINCLKSEL_VAL == 1) || (SYSPLLCLKSEL_VAL == 1) || (EXT_CLOCK_FORCE_ENABLE == 1))
    #if (EXTCLKSEL_VAL == 0)                            // sys_osc_clk is source for external_clk
      #if ((SYSOSCCTRL_VAL) & (1<<SYSOSC_BYPASS) == 0)  // If crystal oscillator not bypassed
        LPC_IOCON->PIO0_8         &= ~(MODE_INACTIVE);  // Disable pull-up and pull-down
        LPC_IOCON->PIO0_9         &= ~(MODE_INACTIVE);  // Disable pull-up and pull-down
        LPC_SWM->PINENABLE0       &= ~(XTALIN|XTALOUT); // Enable XTALIN/XTALOUT func.
      #endif
      LPC_SYSCON->SYSOSCCTRL      = SYSOSCCTRL_VAL;     // Update the SYSOSCCTRL register
      LPC_SYSCON->PDRUNCFG        &= ~(SYSOSC_PD);      // Power-up sysosc
      for (i = 0; i < 200; i++) __NOP();                // Wait for osc to stabilize  
    #elif (EXTCLKSEL_VAL == 1)                          // clk_in is source for external_clk
      LPC_IOCON->PIO0_1           &= ~(MODE_INACTIVE);  // Disable pull-up and pull-down
      LPC_SWM->PINENABLE0         &= ~(CLKIN);          // Enable CLKIN func.
    #endif
  #endif
  
  // Step 1. Choose source for external_clk (either sys_osc_clk or clk_in) 
  LPC_SYSCON->EXTCLKSEL         = EXTCLKSEL_VAL;      // Update the actual register

  // Step 2. Power up and configure the WDT OSC if it's needed for main_clk or sys_pll0_clk,
  // or WDTOSCCTRL_VAL not equal to 0x000 (i.e. WDT OSC needed for other clock source).
  #if ((MAINCLKSEL_VAL == 2) || (SYSPLLCLKSEL_VAL == 2) || (WDTOSCCTRL_VAL != 0x000))
    LPC_SYSCON->WDTOSCCTRL      = WDTOSCCTRL_VAL;     // Update the actual register
    LPC_SYSCON->PDRUNCFG        &= ~(WDTOSC_PD);      // Power up the WDTOSC
    for (i = 0; i < 200; i++) __NOP();                // Wait for osc to stabilize
  #endif

  // Step 3. Configure the system PLL if it's needed for main_clk,
  // or SYSPLLCTRL_VAL not equal to 0x00 (i.e. PLL needed for other clock source)
  #if ((MAINCLKPLLSEL_VAL == 1) || (SYSPLLCTRL_VAL != 0))
    LPC_SYSCON->PDRUNCFG        |= (SYSPLL_PD);       // Power down PLL before changing divider ratio
    LPC_SYSCON->SYSPLLCLKSEL    = SYSPLLCLKSEL_VAL;   // Select the input to the system PLL (sys_pll0_clk_src_i)
    LPC_SYSCON->SYSPLLCLKUEN    = 0;                  // Toggle update register
    LPC_SYSCON->SYSPLLCLKUEN    = 1;
    while (!(LPC_SYSCON->SYSPLLCLKUEN & 1)) __NOP();  // Wait until updated
    LPC_SYSCON->SYSPLLCTRL      = SYSPLLCTRL_VAL;     // Update the SYSPLLCTRL register
    LPC_SYSCON->PDRUNCFG        &= ~(SYSPLL_PD);      // Power up system PLL
    while (!(LPC_SYSCON->SYSPLLSTAT & 1)) __NOP();    // Wait until PLL locked
  #endif

  // Step 4. Choose source for main_clk_pre_pll
  LPC_SYSCON->MAINCLKSEL        = MAINCLKSEL_VAL;     // Update the actual register
  LPC_SYSCON->MAINCLKUEN        = 0;                  // Toggle update register
  LPC_SYSCON->MAINCLKUEN        = 1;
  while (!(LPC_SYSCON->MAINCLKUEN & 1)) __NOP();      // Wait until updated

  // Step 5. Choose source for main_clk, either main_clk_pre_pll (0) or sys_pll0_clk_src_i (1)
  LPC_SYSCON->MAINCLKPLLSEL     = MAINCLKPLLSEL_VAL;  // Update the actual register
  LPC_SYSCON->MAINCLKPLLUEN     = 0;                  // Toggle update register
  LPC_SYSCON->MAINCLKPLLUEN     = 1;
  while (!(LPC_SYSCON->MAINCLKPLLUEN & 1)) __NOP();   // Wait until updated

  // Step 6. Configure the main_clock divider
  LPC_SYSCON->SYSAHBCLKDIV      =  SYSAHBCLKDIV_VAL;  // Update the actual register

  LPC_SYSCON->SYSAHBCLKCTRL[0] &= ~(SWM | IOCON);     // Turn off peripheral clocks before leaving

} // end of SystemInit




//----------------------------------------------------------------------------
//  Global clock variable declarations and initial value assignments
//----------------------------------------------------------------------------
uint32_t main_clk        = __MAIN_CLK;
uint32_t wdt_osc_clk     = __WDT_OSC_CLK;
uint32_t sys_pll0_clk    = __SYS_PLL0_CLK;
uint32_t fro_clk         = __FRO_CLK;
uint32_t fro_div_clk     = __FRO_DIV_CLK;
uint32_t system_ahb_clk  = __SYSTEM_AHB_CLK;

//----------------------------------------------------------------------------
// Function name: SystemCoreClockUpdate
// Determines the actual system_ahb_clk (core clock), main_clock, 
// wdt_osc_clk, sys_pll0_clk, fro_clk, and fro_div_clk frequencies
// based on the current state of the device, and updates the associated 
// global clock variables.
//----------------------------------------------------------------------------
void SystemCoreClockUpdate (void)
{
  volatile uint32_t *faim_addr = (volatile uint32_t *)LPC_FAIM_BASE;
  uint32_t external_clk, main_clk_pre_pll, sys_pll0_clk_src_i;
  uint32_t fro_oscout, fro_clock, fro_src_i;
  uint32_t temp;

  // Set the fro_clk and fro_div_clk variables according to current register settings
  temp = LPC_SYSCON->FROOSCCTRL;
  switch (temp & FRO_FREQSEL_MASK) {
    case 0: fro_oscout = 18000000; break;
    case 1: fro_oscout = 24000000; break;
    default:fro_oscout = 30000000; break;
  }
  fro_clock = 0;
  if ((LPC_SYSCON->PDRUNCFG & (FROOUT_PD | FRO_PD)) == 0x0)
    fro_clock = fro_oscout;
  if ((*faim_addr & (1<<1)) == 0)        // if FRO_LOW_POWER_START bit == 0
    fro_src_i = fro_clock / 2;
  else
    fro_src_i = fro_clock / 16;
  if (((temp >> FRO_DIRECT) & 0x1) == 1) // if fro_direct bit == 1
    fro_clk = fro_clock;
  else
    fro_clk = fro_src_i;
  fro_div_clk = fro_clk / 2;

  // Set the external_clk variable according to current register values
  if (LPC_SYSCON->EXTCLKSEL == 0)
    external_clk = __SYS_OSC_CLK;
  else
    external_clk = __CLKIN_CLK;

  // Set the wdt_osc_clk variable according to current register values, but only if the wdtosc is not in power down
  if (!(LPC_SYSCON->PDRUNCFG & WDTOSC_PD)) {
    switch ((LPC_SYSCON->WDTOSCCTRL >> 5) & 0x0F) {
      case 0:  wdt_osc_clk =       0; break;
      case 1:  wdt_osc_clk =  600000; break;
      case 2:  wdt_osc_clk = 1050000; break;
      case 3:  wdt_osc_clk = 1400000; break;
      case 4:  wdt_osc_clk = 1750000; break;
      case 5:  wdt_osc_clk = 2100000; break;
      case 6:  wdt_osc_clk = 2400000; break;
      case 7:  wdt_osc_clk = 2700000; break;
      case 8:  wdt_osc_clk = 3000000; break;
      case 9:  wdt_osc_clk = 3250000; break;
      case 10: wdt_osc_clk = 3500000; break;
      case 11: wdt_osc_clk = 3750000; break;
      case 12: wdt_osc_clk = 4000000; break;
      case 13: wdt_osc_clk = 4200000; break;
      case 14: wdt_osc_clk = 4400000; break;
      case 15: wdt_osc_clk = 4600000; break;
    }
    wdt_osc_clk /= (((LPC_SYSCON->WDTOSCCTRL & 0x1F) + 1) << 1);
  }
  else {
    wdt_osc_clk = 0;
  }

  // Set the sys_pll0_clk_src_i variable according to current register values, but only if the PLL is locked
  if (LPC_SYSCON->SYSPLLSTAT & 1) {
    switch (LPC_SYSCON->SYSPLLCLKSEL & 0x3) {
      case 0: sys_pll0_clk_src_i = fro_clk;      break;
      case 1: sys_pll0_clk_src_i = external_clk; break;
      case 2: sys_pll0_clk_src_i = wdt_osc_clk;  break;
      case 3: sys_pll0_clk_src_i = fro_div_clk;  break;
    }
  }
  else {
    sys_pll0_clk_src_i = 0;
  }

  // Set the main_clk_pre_pll variable according to current register values
  switch (LPC_SYSCON->MAINCLKSEL & 0x3) {
    case 0: main_clk_pre_pll = fro_clk;      break;
    case 1: main_clk_pre_pll = external_clk; break;
    case 2: main_clk_pre_pll = wdt_osc_clk;  break;
    case 3: main_clk_pre_pll = fro_div_clk;  break;
  }

  // Set the sys_pll0_clk variable according to current register values
  sys_pll0_clk =  sys_pll0_clk_src_i * ((LPC_SYSCON->SYSPLLCTRL & 0x1F) + 1);

  // Set the main_clk variable according to current register values
  switch (LPC_SYSCON->MAINCLKPLLSEL & 0x3) {
    case 0:  main_clk = main_clk_pre_pll; break;
    case 1:  main_clk = sys_pll0_clk;     break;
    default: main_clk = 0;
  }

  // Set the system_ahb_clk (a.k.a SystemCoreClock) variable according to current register values
  system_ahb_clk = main_clk / LPC_SYSCON->SYSAHBCLKDIV;

} // end of SystemCoreClockUpdate
