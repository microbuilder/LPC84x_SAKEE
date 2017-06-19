/*
===============================================================================
 Name        : dac_waveform.h
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
*/
#ifndef DAC_WAVEGEN_H_
#define DAC_WAVEGEN_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
 extern "C" {
#endif

void dac_wavegen_init(void);
void dac_wavegen_run(const uint16_t samples[], uint32_t count, uint32_t freq);
void dac_wavegen_stop(void);

#ifdef __cplusplus
 }
#endif

#endif /* DAC_WAVEGEN_H_ */
