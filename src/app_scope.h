/*
===============================================================================
 Name        : app_scope.h
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
 */
#ifndef APP_SCOPE_H_
#define APP_SCOPE_H_

typedef enum
{
	APP_SCOPE_RATE_500_KHZ = 2,
	APP_SCOPE_RATE_250_KHZ = 4,
	APP_SCOPE_RATE_100_KHZ = 10,
	APP_SCOPE_RATE_50_KHZ  = 20,
	APP_SCOPE_RATE_25_KHZ  = 40,
	APP_SCOPE_RATE_10_KHZ  = 100,
	APP_SCOPE_RATE_5_KHZ   = 200,
	APP_SCOPE_RATE_2_5_KHZ = 400,
	APP_SCOPE_RATE_1_KHZ   = 1000,
	APP_SCOPE_RATE_500_HZ  = 2000,
	APP_SCOPE_RATE_250_HZ  = 4000,
	APP_SCOPE_RATE_100_HZ  = 10000,
	APP_SCOPE_RATE_60_HZ   = 16666,
	APP_SCOPE_RATE_50_HZ   = 20000,
	APP_SCOPE_RATE_25_HZ   = 40000,
	APP_SCOPE_RATE_10_HZ   = 100000
} app_scope_rate_t;

void app_scope_init(app_scope_rate_t rate);
void app_scope_run(void);


#endif /* APP_SCOPE_H_ */
