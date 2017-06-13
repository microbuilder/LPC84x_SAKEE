#include "LPC8xx.h"
#include "uart.h"
#include "syscon.h"
#include "swm.h"
#include "chip_setup.h"



// Implementation of sendchar (used by printf)
// This is for Keil and MCUXpresso projects.
int sendchar (int ch) {
  while (!((pDBGU->STAT) & TXRDY));   // Wait for TX Ready
  return (pDBGU->TXDAT  = ch);        // Write one character to TX data register
}


// Implementation of MyLowLevelPutchar (used by printf)
// This is for IAR projects. Must include locally modified __write in the project.
int MyLowLevelPutchar(int ch) {
  while (!((pDBGU->STAT) & TXRDY));   // Wait for TX Ready
  return (pDBGU->TXDAT  = ch);        // Write one character to TX data register
}


// Implementation of getkey (used by scanf)
// This is for Keil and MCUXpresso projects.
int getkey (void) {
  while (!((pDBGU->STAT) & RXRDY));   // Wait for RX Ready
  return (pDBGU->RXDAT );             // Read one character from RX data register
}


// Implementation of MyLowLevelGetchar (used by scanf)
// This is for IAR projects. Must include locally modified __read in the project.
int MyLowLevelGetchar(void){
  while (!((pDBGU->STAT) & RXRDY));   // Wait for RX Ready
  return (pDBGU->RXDAT );             // Read one character from RX data register
}






//
// Function: setup_debug_uart
//
// UART BRG calculation:
// For asynchronous mode (UART mode) the BRG formula is:
// (BRG + 1) * (1 + (m/256)) * (16 * baudrate Hz.) = FRG_in Hz.
// For this example, we set m = 0 (so FRG = 1). 
// We choose FRG_in = main_clk, using FRG0CLKSEL mux setup below.
// Then, we use the global main_clk variable, as set by the function 
// SystemCoreClockUpdate(), in our BRG calculation as follows:
// BRG = (main_clk Hz. / (16 * desired_baud Hz.)) - 1

void setup_debug_uart() {

  // Select the clock source to FRG0 by writing to the FRG0CLKSEL register
  //LPC_SYSCON->FRG0CLKSEL = 1;         // '1' selects main_clk as input to FRG0

  // Select the function clock source for the USART by writing to the appropriate FCLKSEL register.
  LPC_SYSCON->FCLKSEL[INDEX] = FCLKSEL_MAIN_CLK;     // Select main_clk as fclk to this USART

  // Turn on relevant peripheral APB/AHB clocks 
  LPC_SYSCON->SYSAHBCLKCTRL[0] |= (DBGU | SWM);

  // Connect USART TXD, RXD signals to port pins
  ConfigSWM(DBGUTXD, DBGTXPIN);
  ConfigSWM(DBGURXD, DBGRXPIN);
	
  // Give the USART a reset
  LPC_SYSCON->PRESETCTRL[0] &= (DBGURST);
  LPC_SYSCON->PRESETCTRL[0] |= ~(DBGURST);

  // Get the Main Clock frequency for the BRG calculation.
  SystemCoreClockUpdate();
	
  // Write calculation result to BRG register
  pDBGU->BRG = (main_clk / (16 * DBGBAUDRATE)) - 1;

  // Configure the USART CFG register:
  // 8 data bits, no parity, one stop bit, no flow control, asynchronous mode
  pDBGU->CFG = DATA_LENG_8|PARITY_NONE|STOP_BIT_1;

  // Configure the USART CTL register (nothing to be done here)
  // No continuous break, no address detect, no Tx disable, no CC, no CLRCC
  pDBGU->CTL = 0;

  // Clear any pending flags (for illustration, isn't necessary after the peripheral reset)
  pDBGU->STAT = 0xFFFF;

  // Enable the USART RX Ready Interrupt (add these lines to main if the project assumes an interrupt-driven use case)
  //pDBGU->INTENSET = RXRDY;
  //NVIC_EnableIRQ(DBGUIRQ);

  // Enable USART
  pDBGU->CFG |= UART_EN;
	
  // Turn off SWM clock before returning
  LPC_SYSCON->SYSAHBCLKCTRL[0] &= ~(SWM);

}

