/*
 * defines.h
 *
 *  Created on: 20 нояб. 2022 г.
 *      Author: DrDeLaBill
 */

#ifndef INC_DEFINES_H_
#define INC_DEFINES_H_


#ifdef __cplusplus
extern "C" {
#endif



#include <stdint.h>

#include "stm32f1xx_hal.h"

// General settings
#define FW_VERSION            ((uint32_t)2)
#define CF_VERSION_DEFAULT    ((uint32_t)1)
#define MILLIS_IN_SECOND      1000
#define DEFAULT_UART_DELAY    100
#define DEFAULT_UART_SIZE     100
#define CHAR_SETIINGS_SIZE    30
#define CHAR_PARAM_SIZE       30
#define DEFAULT_SLEEPING_TIME 900000
#define MIN_TANK_VOLUME       3126
#define MAX_TANK_VOLUME       75
#define MIN_TANK_LTR          20000
#define MAX_TANK_LTR          375000
#define SETTING_VALUE_MIN     0
#define MAX_ADC_VALUE         4095
#define LOW_VOLTAGE           0
#define HIGH_VOLTAGE          3.3
#define ADC_READ_TIMEOUT      100
#define MILLILITERS_IN_LITER  1000

#define proxy_log(level, fmt, ...) _proxy_log(level, fmt"\n", ##__VA_ARGS__)

typedef struct {
	uint8_t year;
	uint8_t month;
	uint8_t date;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} DateTime;


#ifdef __cplusplus
}
#endif


#endif /* INC_DEFINES_H_ */
