/*
 * clock.c
 *
 *  Created on: Jul 27, 2023
 *      Author: DrDeLaBill
 */
#include <clock_service.h>

#include <stdbool.h>

#include "stm32f1xx_hal.h"
#include "rtc.h"

#include "main.h"


uint8_t get_year()
{
	RTC_DateTypeDef date;
	HAL_StatusTypeDef res = HAL_RTC_GetDate(&CLOCK_RTC, &date, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
	    return 0;
	}
	return date.Year;
}

uint8_t get_month()
{
	RTC_DateTypeDef date;
	HAL_StatusTypeDef res = HAL_RTC_GetDate(&CLOCK_RTC, &date, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
	    return 0;
	}
	return date.Month;
}

uint8_t get_date()
{
	RTC_DateTypeDef date;
	HAL_StatusTypeDef res = HAL_RTC_GetDate(&CLOCK_RTC, &date, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
	    return 0;
	}
	return date.Date;
}

uint8_t get_hour()
{
	RTC_TimeTypeDef time;
	HAL_StatusTypeDef res = HAL_RTC_GetTime(&CLOCK_RTC, &time, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
	    return 0;
	}
	return time.Hours;
}

uint8_t get_minute()
{
	RTC_TimeTypeDef time;
	HAL_StatusTypeDef res = HAL_RTC_GetTime(&CLOCK_RTC, &time, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
	    return 0;
	}
	return time.Minutes;
}

uint8_t get_second()
{
	RTC_TimeTypeDef time;
	HAL_StatusTypeDef res = HAL_RTC_GetTime(&CLOCK_RTC, &time, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
	    return 0;
	}
	return time.Seconds;
}

bool save_date(RTC_DateTypeDef* date)
{
	if (date->Date > 31) {
		date->Date = 1;
	}
	if (date->Month > 12) {
		date->Month = 1;
	}
	return HAL_RTC_SetDate(&CLOCK_RTC, date, RTC_FORMAT_BIN) == HAL_OK;
}

bool save_time(RTC_TimeTypeDef* time)
{
	if (time->Hours > 23) {
		time->Hours = 0;
	}
	if (time->Minutes > 59) {
		time->Minutes = 0;
	}
	if (time->Seconds > 59) {
		time->Seconds = 0;
	}
	return HAL_RTC_SetTime(&CLOCK_RTC, time, RTC_FORMAT_BIN) == HAL_OK;
}
