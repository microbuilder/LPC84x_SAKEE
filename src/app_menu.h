/*
===============================================================================
 Name        : app_menu.h
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
*/

#ifndef APP_MENU_H_
#define APP_MENU_H_

typedef enum
{
	APP_MENU_OPTION_ABOUT = 0,
	APP_MENU_OPTION_VOLTMETER = 1,
	APP_MENU_OPTION_SCOPE = 2,
	APP_MENU_OPTION_I2CSCANNER = 3,
	APP_MENU_OPTION_WAVEGEN = 4,
	APP_MENU_OPTION_CONTINUITY = 5,
	APP_MENU_OPTION_LAST
} app_menu_option_t;

void app_menu_init(void);
app_menu_option_t app_menu_wait(void);

#endif /* APP_MENU_H_ */
