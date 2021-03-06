/*
===============================================================================
 Name        : adc.h
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
*/

#include <stdio.h>

#ifndef ADC_POLL_H_
#define ADC_POLL_H_

int      adc_poll_init(void);
uint16_t adc_poll_read(uint8_t adc_ch);

#endif /* ADC_POLL_H_ */
