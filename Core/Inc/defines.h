/*
 * defines.h
 *
 *  Created on: 20 нояб. 2022 г.
 *      Author: georgy
 */

#ifndef INC_DEFINES_H_
#define INC_DEFINES_H_

#include "stm32f1xx_hal.h"

// General settings
#define FW_VERSION         1
#define CF_VERSION_DEFAULT 1
#define MILLIS_IN_SECOND   1000
#define DEFAULT_UART_DELAY 100
#define DEFAULT_UART_SIZE  100
#define CHAR_SETIINGS_SIZE 20
#define CHAR_PARAM_SIZE    30

#define proxy_log(level, fmt, ...) _proxy_log(level, fmt"\n", ##__VA_ARGS__)

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t date;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} DateTime;

#endif /* INC_DEFINES_H_ */
