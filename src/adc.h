/*
 * adc.h
 *
 *  Created on: Apr 3, 2017
 *      Author: ktown
 */

#include <stdio.h>

#ifndef ADC_H_
#define ADC_H_

int adc_init(void);
uint16_t adc_read(uint8_t adc_ch);

#endif /* ADC_H_ */
