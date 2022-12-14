/*
 * defines.h
 *
 *  Created on: 20 нояб. 2022 г.
 *      Author: georgy
 */

#ifndef INC_DEFINES_H_
#define INC_DEFINES_H_

// General settings
#define DEBUG true
#define CHAR_SETIINGS_SIZE 20
#define CHAR_MESSAGE_SIZE 100
#define CHAR_LONG_MESSAGE_SIZE 550
#define DEFAULT_SLEEPING_TIME 900000 // 15 min
#define MILLIS_IN_SECOND 1000
// Liters
#define MIN_TANK_VOLUME 500
#define MAX_TANK_VOLUME 3500
#define DEFAULT_UART_DELAY 100
#define proxy_log(level, fmt, ...) _proxy_log(level, fmt"\n", ##__VA_ARGS__)
#define SETTING_VALUE_MIN 0
#define MAX_ADC_VALUE 4095
#define LIQUID_ERROR -1.0
#define MILLILITERS_IN_LITER 1000
// Pump
#define MIN_PUMP_WORK_TIME 60 // 1 min
#define CYCLES_PER_HOUR 4
#define MONTHS_PER_YEAR 12
#define HOURS_PER_DAY 24
#define MINUTES_PER_HOUR 60
#define SECONDS_PER_MINUTE 60
// Sim module
#define REQUEST_JSON_SIZE 200

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t date;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} DateTime;

#endif /* INC_DEFINES_H_ */
