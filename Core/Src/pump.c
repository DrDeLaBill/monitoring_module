/*
 * pump.c
 *
 *  Created on: Oct 27, 2022
 *      Author: georgy
 */

#include "pump.h"

#include <stdlib.h>

#include "settings.h"
#include "defines.h"
#include "utils.h"
#include "liquid_sensor.h"
#include "ds1307_for_stm32_hal.h"
#include "sim_module.h"


#define CYCLES_PER_HOUR    4
#define MONTHS_PER_YEAR    12
#define HOURS_PER_DAY      24
#define SECONDS_PER_MINUTE 60
#define MIN_PUMP_WORK_TIME SECONDS_PER_MINUTE
#define MINUTES_PER_HOUR   60
#define LOG_SIZE           140


void _calculate_work_time();
void _calculate_pause_time();
void _set_current_time(DateTime *datetime);
void _filterTime(DateTime *targetTime);
uint32_t _daytime_to_int(DateTime *datetime);
bool _if_time_to_start_pump();
bool _if_time_to_stop_pump();
bool _if_pump_work_time_too_short();
void _start_pump();
void _stop_pump();
uint8_t _days_in_month(uint8_t year, uint8_t month);
bool _is_leap_year(uint8_t year);
void _write_work_time_to_log();


const char* PUMP_TAG = "\r\nPUMP";

uint8_t current_state = RESET;
DateTime startTime = {};
DateTime stopTime = {};


void pump_init()
{
	HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, RESET);
	current_state = RESET;
//	HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, SET);
	_set_current_time(&startTime);
	if (module_settings.milliliters_per_day == 0) {
		LOG_DEBUG(PUMP_TAG, "No setting: milliliters_per_day\r\n");
		return;
	}
	if (module_settings.pump_speed == 0) {
		LOG_DEBUG(PUMP_TAG, "No setting: pump_speed\r\n");
		return;
	}
	_calculate_work_time();
}

void pump_proccess()
{
	if (module_settings.module_enabled == 0) {
		_stop_pump();
		return;
	}
	if (module_settings.milliliters_per_day == 0) {
		return;
	}
	if (module_settings.pump_speed == 0) {
		return;
	}
	if (_if_time_to_stop_pump()) {
		_calculate_pause_time();
		_stop_pump();
		pump_show_work();
	}
	if (_if_pump_work_time_too_short()) {
		return;
	}
	if (_if_time_to_start_pump()) {
		_calculate_work_time();
		_start_pump();
		_write_work_time_to_log();
		pump_show_work();
	}
}

void _calculate_work_time()
{
	uint8_t needed_time = module_settings.milliliters_per_day * MINUTES_PER_HOUR / HOURS_PER_DAY / module_settings.pump_speed / CYCLES_PER_HOUR;
	if (needed_time > (MINUTES_PER_HOUR / CYCLES_PER_HOUR)) {
		needed_time = MINUTES_PER_HOUR / CYCLES_PER_HOUR;
	}
	_set_current_time(&stopTime);
	_set_current_time(&startTime);
	stopTime.minute += needed_time;
	_filterTime(&stopTime);
}

void _calculate_pause_time()
{
	startTime.minute += MINUTES_PER_HOUR / CYCLES_PER_HOUR;
	_filterTime(&startTime);
}

void _set_current_time(DateTime *datetime)
{
	datetime->year = DS1307_GetYear();
	datetime->month = DS1307_GetMonth();
	datetime->date = DS1307_GetDate();
	datetime->hour = DS1307_GetHour();
	datetime->minute = DS1307_GetMinute();
	datetime->second = DS1307_GetSecond();
}

void _filterTime(DateTime *targetTime)
{
	if (targetTime->second >= SECONDS_PER_MINUTE) {
		targetTime->minute += 1;
		targetTime->second -= SECONDS_PER_MINUTE;
	}
	if (targetTime->minute >= MINUTES_PER_HOUR) {
		targetTime->hour += 1;
		targetTime->minute -= MINUTES_PER_HOUR;
	}
	if (targetTime->hour >= HOURS_PER_DAY) {
		targetTime->date += 1;
		targetTime->hour -= HOURS_PER_DAY;
	}
	if (targetTime->date > _days_in_month(targetTime->year, targetTime->month)) {
		targetTime->month += 1;
		targetTime->date = 1;
	}
	if (targetTime->month > MONTHS_PER_YEAR) {
		targetTime->year += 1;
		targetTime->month = 1;
	}
}

bool _if_pump_work_time_too_short()
{
	long long start_time = _daytime_to_int(&startTime),
			  stop_time = _daytime_to_int(&stopTime);
	return abs(start_time - stop_time) < MIN_PUMP_WORK_TIME;
}

uint32_t _daytime_to_int(DateTime *datetime)
{
	return datetime->second +
		   datetime->minute * SECONDS_PER_MINUTE +
		   datetime->hour * SECONDS_PER_MINUTE * MINUTES_PER_HOUR +
		   datetime->date * SECONDS_PER_MINUTE * MINUTES_PER_HOUR * HOURS_PER_DAY;
}

bool _if_time_to_start_pump()
{
	if (current_state == SET) {
		return false;
	}
	return DS1307_GetYear()    >= startTime.year &&
		   DS1307_GetMonth()   >= startTime.month &&
		   DS1307_GetDate()    >= startTime.date &&
		   DS1307_GetHour()    >= startTime.hour &&
		   DS1307_GetMinute()  >= startTime.minute &&
		   DS1307_GetSecond()  >= startTime.second;
}

bool _if_time_to_stop_pump()
{
	if (current_state == RESET) {
		return false;
	}
	return DS1307_GetYear()    >= stopTime.year &&
		   DS1307_GetMonth()   >= stopTime.month &&
		   DS1307_GetDate()    >= stopTime.date &&
		   DS1307_GetHour()    >= stopTime.hour &&
		   DS1307_GetMinute()  >= stopTime.minute &&
		   DS1307_GetSecond()  >= stopTime.second;
}

void _start_pump()
{
	if (current_state == SET) {
		return;
	}
	current_state = SET;
	LOG_DEBUG(PUMP_TAG,	"PUMP ON\r\n");
	HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, current_state);
}

void _stop_pump()
{
	if (current_state == RESET) {
		return;
	}
	current_state = RESET;
	LOG_DEBUG(PUMP_TAG, "PUMP OFF\r\n");
	HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, current_state);
}

void pump_show_work()
{
	LOG_DEBUG(
		PUMP_TAG,
		"\n"
		"start %u-%02u-%02uT%02u:%02u:%02u\n"
		"stop  %u-%02u-%02uT%02u:%02u:%02u\n"
		"\n"
		"now   %u-%02u-%02uT%02u:%02u:%02u\n",
		startTime.year,
		startTime.month,
		startTime.date,
		startTime.hour,
		startTime.minute,
		startTime.second,
		stopTime.year,
		stopTime.month,
		stopTime.date,
		stopTime.hour,
		stopTime.minute,
		stopTime.second,
		DS1307_GetYear(),
		DS1307_GetMonth(),
		DS1307_GetDate(),
		DS1307_GetHour(),
		DS1307_GetMinute(),
		DS1307_GetSecond()
	);
}

void _write_work_time_to_log()
{
	uint32_t start_time = _daytime_to_int(&startTime),
			 stop_time  = _daytime_to_int(&stopTime);
	module_settings.pump_work_seconds += abs(stop_time - start_time);
	settings_save();
}

uint8_t _days_in_month(uint8_t year, uint8_t month)
{
	enum MonthIndex
	{
	    Jan = 1, Feb = 2, Mar = 3, Apr = 4,  May = 5,  Jun = 6,
	    Jul = 7, Aug = 8, Sep = 9, Oct = 10, Nov = 11, Dec = 12
	};
	uint8_t number_of_days;
    switch (month) {
		case Jan:
		case Mar:
		case May:
		case Jul:
		case Aug:
		case Oct:
		case Dec:
			number_of_days = 31;
			break;
		case Apr:
		case Jun:
		case Sep:
		case Nov:
			number_of_days = 30;
			break;
		case Feb:
			number_of_days = 28;
			if (_is_leap_year(year)) {
				number_of_days = 29;
			}
			break;
		default:
			number_of_days = 0;
			break;
    }
    return number_of_days;
}

bool _is_leap_year(uint8_t year)
{
    return year % 4 == 0;
}


