/*
===============================================================================
 Name        : app_wavegen.h
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
 */

#ifndef APP_WAVEGEN_H_
#define APP_WAVEGEN_H_


#ifdef __cplusplus
 extern "C" {
#endif

typedef enum
{
	APP_WAVEGEN_RATE_800_HZ  = 800,
	APP_WAVEGEN_RATE_400_HZ  = 400,
	APP_WAVEGEN_RATE_200_HZ  = 200,
	APP_WAVEGEN_RATE_100_HZ  = 100,
	APP_WAVEGEN_RATE_50_HZ   = 50,
	APP_WAVEGEN_RATE_25_HZ   = 25,
	APP_WAVEGEN_RATE_LAST    = 0
} app_wavegen_rate_t;

void app_wavegen_init(void);
void app_wavegen_run(void);

#ifdef __cplusplus
 }
#endif

#endif /* APP_WAVEGEN_H_ */
