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

#ifndef ADC_H_
#define ADC_H_

int      adc_poll_init(void);
uint16_t adc_poll_read(uint8_t adc_ch);

#endif /* ADC_H_ */
