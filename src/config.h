/**************************************************************************/
/*!
    @file     config.h
    @author   hathach

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2017, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/
#ifndef CONFIG_H_
#define CONFIG_H_


#ifdef __cplusplus
 extern "C" {
#endif

#include "swm.h"

#define MV_PER_LSB    (3300.0F / 0xFFF)

// PIN config
#define LED_PIN     (P0_0)

#define BUTTON_PIN  (P0_19)
#define QEI_A_PIN   (P0_20)
#define QEI_B_PIN   (P0_21)

#define ADC_CHANNEL (2) // Pin P0.14 (A0)
#define WAVEGEN_DAC (1) // 0 = P0.17/ANALOG4, 1 = 0.29/ANALOG5

// Common functions
#define bit(_n)               ( 1 << (_n) )
#define bit_test(_x, _n)      ( ((_x) & bit(_n)) ? 1 : 0 )
#define bit_is_set(_x, _n)    bit_test(_x, _n)
#define bit_is_clear(_x, _n)  (!bit_test(_x, _n))

#ifdef __cplusplus
 }
#endif

#endif /* CONFIG_H_ */
