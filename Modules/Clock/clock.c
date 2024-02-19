/* Copyright © 2023 Georgy E. All rights reserved. */

#include "clock.h"

#include <stdint.h>
#include <stdbool.h>

#include "log.h"
#include "bmacro.h"
#include "hal_defs.h"
#include "ds1307_driver.h"


uint8_t clock_get_year()
{
	return DS1307_GetYear();
}

uint8_t clock_get_month()
{
	return DS1307_GetMonth();
}

uint8_t clock_get_date()
{
	return DS1307_GetDate();
}

uint8_t clock_get_hour()
{
	return DS1307_GetHour();
}

uint8_t clock_get_minute()
{
	return DS1307_GetMinute();
}

uint8_t clock_get_second()
{
	return DS1307_GetSecond();
}

void clock_save_time(TimeTypeDef* time)
{
	DS1307_SetHour(time->Hours);
	DS1307_SetMinute(time->Minutes);
	DS1307_SetSecond(time->Seconds);
}

void clock_save_date(DateTypeDef* date)
{
	DS1307_SetYear(date->Year);
	DS1307_SetMonth(date->Month);
	DS1307_SetDate(date->Date);
}

void clock_get_rtc_time(TimeTypeDef* time)
{
	time->Hours = DS1307_GetHour();
	time->Minutes = DS1307_GetMinute();
	time->Seconds = DS1307_GetSecond();
}

void clock_get_rtc_date(DateTypeDef* date)
{
	date->Year = DS1307_GetYear();
	date->Month = DS1307_GetMonth();
	date->Date = DS1307_GetDate();
}


enum Months {
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
};

uint32_t clock_datetime_to_seconds(DateTypeDef* date, TimeTypeDef* time)
{
	uint32_t days = date->Year * 365;
	if (date->Year > 0) {
		days += (date->Year / 4) + 1;
	}
	for (unsigned i = 0; i < (unsigned)(date->Month > 0 ? date->Month - 1 : 0); i++) {
		switch (i) {
		case JANUARY:
			days += 31;
			break;
		case FEBRUARY:
			days += ((date->Year % 4 == 0) ? 29 : 28);
			break;
		case MARCH:
			days += 31;
			break;
		case APRIL:
			days += 30;
			break;
		case MAY:
			days += 31;
			break;
		case JUNE:
			days += 30;
			break;
		case JULY:
			days += 31;
			break;
		case AUGUST:
			days += 31;
			break;
		case SEPTEMBER:
			days += 30;
			break;
		case OCTOBER:
			days += 31;
			break;
		case NOVEMBER:
			days += 30;
			break;
		case DECEMBER:
			days += 31;
			break;
		default:
			break;
		};
	}
	if (date->Date > 0) {
		days += (date->Date - 1);
	}
	uint32_t hours = days * 24 + time->Hours;
	uint32_t minutes = hours * 60 + time->Minutes;
	uint32_t seconds = minutes * 60 + time->Seconds;
	return seconds;
}

uint32_t clock_get_timestamp()
{
	DateTypeDef date = {0};
	TimeTypeDef time = {0};

	clock_get_rtc_date(&date);
	clock_get_rtc_time(&time);

	return clock_datetime_to_seconds(&date, &time);
}
