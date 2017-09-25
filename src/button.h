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

#endif /* BUTTON_H_ */
