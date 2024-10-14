/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#include "soul.h"

//#define NO_SYSTEM_RTC_TEST
//#define NO_SYSTEM_I2C_RESET


#ifdef DEBUG
#   define SYSTEM_BEDUG (1)
#endif


extern uint16_t SYSTEM_ADC_VOLTAGE[3];


void system_clock_hsi_config(void);

void system_rtc_test(void);

void system_pre_load(void);
void system_post_load(void);

void system_error_handler(SOUL_STATUS error, void (*error_loop) (void));

uint32_t get_system_power(void);

void system_reset_i2c_errata(void);

char* get_system_serial_str(void);


#ifdef __cplusplus
}
#endif


#endif
