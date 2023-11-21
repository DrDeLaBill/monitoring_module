/*
 * clock.c
 *
 *  Created on: Jul 27, 2023
 *      Author: DrDeLaBill
 */
#include <clock_service.h>

#include <stdbool.h>

#include "stm32f1xx_hal.h"

#include "main.h"
#include "ds1307_driver.h"


uint8_t _get_days_in_month(uint8_t month);

uint16_t get_year()
{
	return DS1307_GetYear();
}

uint8_t get_month()
{
	return DS1307_GetMonth();
}

uint8_t get_date()
{
	return DS1307_GetDate();
}

uint8_t get_hour()
{
	return DS1307_GetHour();
}

uint8_t get_minute()
{
	return DS1307_GetMinute();
}

uint8_t get_second()
{
	return DS1307_GetSecond();
}

bool save_datetime(DateTime* datetime)
{
	if (datetime->second > 59) {
		return false;
	}
	if (datetime->minute > 59) {
		return false;
	}
	if (datetime->hour > 23) {
		return false;
	}
	if (datetime->month > 12) {
		return false;
	}
	if (datetime->date > _get_days_in_month(datetime->month)) {
		return false;
	}
	DS1307_SetYear(datetime->year);
	DS1307_SetMonth(datetime->month);
	DS1307_SetDate(datetime->date);
	DS1307_SetHour(datetime->hour);
	DS1307_SetMinute(datetime->minute);
	DS1307_SetSecond(datetime->second);
	return true;
}

uint8_t _get_days_in_month(uint8_t month) {
	switch (month) {
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			return 31;
		case 2:
			return get_year() % 4 ? 28 : 29;
		case 4:
		case 6:
		case 9:
		case 11:
			return 30;
		default:
			return 0;
	}
}
