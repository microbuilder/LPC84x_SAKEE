/*
 * adc_dma.h
 *
 *  Created on: Apr 26, 2017
 *      Author: hathach
 */

#ifndef ADC_DMA_H_
#define ADC_DMA_H_

#include <stdint.h>
#include <stdbool.h>
#include "LPC8xx.h"

#define DMA_BUFFER_SIZE 1024

void adc_dma_init(void);
void adc_dma_start(void);

#endif /* ADC_DMA_H_ */
