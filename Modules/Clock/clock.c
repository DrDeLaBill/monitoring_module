/* Copyright © 2023 Georgy E. All rights reserved. */

#include "clock.h"

#include <stdint.h>
#include <stdbool.h>

#include "glog.h"
#include "bmacro.h"
#include "hal_defs.h"
#include "ds1307_driver.h"


extern RTC_HandleTypeDef hrtc;


typedef enum _Months {
	JANUARY = 0,
	FEBRUARY,
	MARCH,
	APRIL,
	MAY,
	JUNE,
	JULY,
	AUGUST,
	SEPTEMBER,
	OCTOBER,
	NOVEMBER,
	DECEMBER
} Months;

uint8_t _get_days_in_month(uint16_t year, Months month);


uint16_t clock_get_year()
{
	uint16_t year = 0;
	if (DS1307_GetYear(&year) != DS1307_OK) {
		year = 0;
	}
	return year;
}

uint8_t clock_get_month()
{
	uint8_t month = 0;
	if (DS1307_GetMonth(&month) != DS1307_OK) {
		month = 0;
	}
	return month;
}

uint8_t clock_get_date()
{
	uint8_t date = 0;
	if (DS1307_GetDate(&date) != DS1307_OK) {
		date = 0;
	}
	return date;
}

uint8_t clock_get_hour()
{
	uint8_t hour = 0;
	if (DS1307_GetHour(&hour) != DS1307_OK) {
		hour = 0;
	}
	return hour;
}

uint8_t clock_get_minute()
{
	uint8_t minute = 0;
	if (DS1307_GetMinute(&minute) != DS1307_OK) {
		minute = 0;
	}
	return minute;
}

uint8_t clock_get_second()
{
	uint8_t second = 0;
	if (DS1307_GetSecond(&second) != DS1307_OK) {
		second = 0;
	}
	return second;
}

bool clock_save_time(const clock_time_t* time)
{
    if (time->Seconds >= SECONDS_PER_MINUTE ||
		time->Minutes >= MINUTES_PER_HOUR ||
		time->Hours   >= HOURS_PER_DAY
	) {
        return false;
    }
	if (DS1307_SetHour(time->Hours) != DS1307_OK) {
		return false;
	}
	if (DS1307_SetMinute(time->Minutes) != DS1307_OK) {
		return false;
	}
	if (DS1307_SetSecond(time->Seconds) != DS1307_OK) {
		return false;
	}
	return true;
}

bool clock_save_date(const clock_date_t* date)
{
	if (date->Date > DAYS_PER_MONTH_MAX || date->Month > MONTHS_PER_YEAR) {
		return false;
	}
	if (DS1307_SetYear((uint16_t)date->Year) != DS1307_OK) {
		return false;
	}
	if (DS1307_SetMonth(date->Month) != DS1307_OK) {
		return false;
	}
	if (DS1307_SetDate(date->Date) != DS1307_OK) {
		return false;
	}
	return true;
}

bool clock_get_rtc_time(clock_time_t* time)
{
	if (DS1307_GetHour(&time->Hours) != DS1307_OK) {
		return false;
	}
	if (DS1307_GetMinute(&time->Minutes) != DS1307_OK) {
		return false;
	}
	if (DS1307_GetSecond(&time->Seconds) != DS1307_OK) {
		return false;
	}
	return true;
}

bool clock_get_rtc_date(clock_date_t* date)
{
	if (DS1307_GetYear(&date->Year) != DS1307_OK) {
		return false;
	}
	if (DS1307_GetMonth(&date->Month) != DS1307_OK) {
		return false;
	}
	if (DS1307_GetDate(&date->Date) != DS1307_OK) {
		return false;
	}
	return true;
}


uint32_t clock_datetime_to_seconds(const clock_date_t* date, const clock_time_t* time)
{
	uint16_t year = date->Year % 100;
	uint32_t days = year * DAYS_PER_YEAR;
	if (year > 0) {
		days += (uint32_t)((year - 1) / LEAP_YEAR_PERIOD) + 1;
	}
	for (unsigned i = 0; i < (unsigned)(date->Month > 0 ? date->Month - 1 : 0); i++) {
		days += _get_days_in_month(year, i);
	}
	days += date->Date;
	days -= 1;
	uint32_t hours = days * HOURS_PER_DAY + time->Hours;
	uint32_t minutes = hours * MINUTES_PER_HOUR + time->Minutes;
	uint32_t seconds = minutes * SECONDS_PER_MINUTE + time->Seconds;
	return seconds;
}

uint32_t clock_get_timestamp()
{
	clock_date_t date = {0};
	clock_time_t time = {0};

	if (!clock_get_rtc_date(&date)) {
#if CLOCK_BEDUG
		BEDUG_ASSERT(false, "Unable to get current date");
#endif
		memset((void*)&date, 0, sizeof(date));
	}

	if (!clock_get_rtc_time(&time)) {
#if CLOCK_BEDUG
		BEDUG_ASSERT(false, "Unable to get current time");
#endif
		memset((void*)&time, 0, sizeof(time));
	}

	return clock_datetime_to_seconds(&date, &time);
}

void clock_seconds_to_datetime(const uint32_t seconds, clock_date_t* date, clock_time_t* time)
{
	memset(date, 0, sizeof(clock_date_t));
	memset(time, 0, sizeof(clock_time_t));

	time->Seconds = (uint8_t)(seconds % SECONDS_PER_MINUTE);
	uint32_t minutes = seconds / SECONDS_PER_MINUTE;

	time->Minutes = (uint8_t)(minutes % MINUTES_PER_HOUR);
	uint32_t hours = minutes / MINUTES_PER_HOUR;

	time->Hours = (uint8_t)(hours % HOURS_PER_DAY);
	uint32_t days = 1 + hours / HOURS_PER_DAY;

	date->WeekDay = (uint8_t)((RTC_WEEKDAY_THURSDAY + days) % (DAYS_PER_WEEK)) + 1;
	if (date->WeekDay == DAYS_PER_WEEK) {
		date->WeekDay = 0;
	}
	date->Month = 1;
	while (days) {
		uint16_t days_in_year = (date->Year % LEAP_YEAR_PERIOD > 0) ? DAYS_PER_YEAR : DAYS_PER_LEAP_YEAR;
		if (days > days_in_year) {
			days -= days_in_year;
			date->Year++;
			continue;
		}

		uint8_t days_in_month = _get_days_in_month(date->Year, date->Month - 1);
		if (days > days_in_month) {
			days -= days_in_month;
			date->Month++;
			continue;
		}

		date->Date = (uint8_t)days;
		break;
	}
}

char* get_clock_time_format()
{
	static char format_time[30] = "";
	memset(format_time, '-', sizeof(format_time) - 1);
	format_time[sizeof(format_time) - 1] = 0;

	clock_date_t date = {0};
	clock_time_t time = {0};

	if (!clock_get_rtc_date(&date)) {
#if CLOCK_BEDUG
		BEDUG_ASSERT(false, "Unable to get current date");
#endif
		memset((void*)&date, 0, sizeof(date));
		return format_time;
	}

	if (!clock_get_rtc_time(&time)) {
#if CLOCK_BEDUG
		BEDUG_ASSERT(false, "Unable to get current time");
#endif
		memset((void*)&time, 0, sizeof(time));
		return format_time;
	}

	snprintf(
		format_time,
		sizeof(format_time) - 1,
		"%u-%02u-%02uT%02u:%02u:%02u",
		date.Year,
		date.Month,
		date.Date,
		time.Hours,
		time.Minutes,
		time.Seconds
	);

	return format_time;
}

char* get_clock_time_format_by_sec(uint32_t seconds)
{
	static char format_time[30] = "";
	memset(format_time, '-', sizeof(format_time) - 1);
	format_time[sizeof(format_time) - 1] = 0;

	clock_date_t date = {0};
	clock_time_t time = {0};

	clock_seconds_to_datetime(seconds, &date, &time);

	snprintf(
		format_time,
		sizeof(format_time) - 1,
		"20%02u-%02u-%02uT%02u:%02u:%02u",
		date.Year,
		date.Month,
		date.Date,
		time.Hours,
		time.Minutes,
		time.Seconds
	);

	return format_time;
}

bool set_clock_ready(bool value)
{
	return DS1307_SetInitialized(value) == DS1307_OK;
}

bool is_clock_ready()
{
	uint8_t value = 0;
	if (DS1307_GetInitialized(&value) != DS1307_OK) {
		return false;
	}
	return (bool)value;
}

uint8_t _get_days_in_month(uint16_t year, Months month)
{
	switch (month) {
	case JANUARY:
		return 31;
	case FEBRUARY:
		return ((year % 4 == 0) ? 29 : 28);
	case MARCH:
		return 31;
	case APRIL:
		return 30;
	case MAY:
		return 31;
	case JUNE:
		return 30;
	case JULY:
		return 31;
	case AUGUST:
		return 31;
	case SEPTEMBER:
		return 30;
	case OCTOBER:
		return 31;
	case NOVEMBER:
		return 30;
	case DECEMBER:
		return 31;
	default:
		break;
	};
	return 0;
}
