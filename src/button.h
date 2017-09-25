/*
===============================================================================
 Name        : button.h
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
*/

#ifndef BUTTON_H_
#define BUTTON_H_

#define BUTTON_USE_CAPTOUCH     1

#define CAPT_PAD_0    0
#define CAPT_PAD_1    1

uint32_t button_pressed(void);
void     button_init(void);

#if BUTTON_USE_CAPTOUCH
#define capt_nvic_enable()    NVIC_EnableIRQ(CAPT_IRQn)
#define capt_nvic_disable()   do { extern volatile bool touching; NVIC_DisableIRQ(CAPT_IRQn); touching = 0; } while(0)
#else
#define capt_nvic_enable()
#define capt_nvic_disable()
#endif

#endif /* BUTTON_H_ */
