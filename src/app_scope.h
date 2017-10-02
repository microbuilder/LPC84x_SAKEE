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
	APP_SCOPE_RATE_10_HZ   = 0,
	APP_SCOPE_RATE_25_HZ,
	APP_SCOPE_RATE_50_HZ,
	APP_SCOPE_RATE_60_HZ,
	APP_SCOPE_RATE_100_HZ,
	APP_SCOPE_RATE_250_HZ,
	APP_SCOPE_RATE_500_HZ,
	APP_SCOPE_RATE_1_KHZ,
	APP_SCOPE_RATE_2_5_KHZ,
	APP_SCOPE_RATE_5_KHZ,
	APP_SCOPE_RATE_10_KHZ,
	APP_SCOPE_RATE_25_KHZ,
	APP_SCOPE_RATE_50_KHZ,
	APP_SCOPE_RATE_100_KHZ,
	APP_SCOPE_RATE_250_KHZ,
	APP_SCOPE_RATE_500_KHZ,
	APP_SCOPE_RATE_LAST
} app_scope_rate_t;

void app_scope_init(void);
void app_scope_run(void);


#endif /* APP_SCOPE_H_ */
