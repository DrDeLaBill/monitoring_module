/*
 * defines.h
 *
 *  Created on: 20 нояб. 2022 г.
 *      Author: georgy
 */

#ifndef INC_DEFINES_H_
#define INC_DEFINES_H_

// General settings
#define MILLIS_IN_SECOND   1000
#define DEFAULT_UART_DELAY 100
#define CHAR_SETIINGS_SIZE 20

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
