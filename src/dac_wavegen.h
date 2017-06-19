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

void dac_wavegen_init(uint8_t dac_id);
void dac_wavegen_run(uint8_t dac_id, const uint16_t samples[], uint32_t count, uint32_t freq);
void dac_wavegen_stop(uint8_t dac_id);

#ifdef __cplusplus
 }
#endif

#endif /* DAC_WAVEGEN_H_ */
