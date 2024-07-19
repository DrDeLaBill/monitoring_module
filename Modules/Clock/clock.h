/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _CLOCK_H_
#define _CLOCK_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "hal_defs.h"


#define SECONDS_PER_MINUTE (60)
#define MINUTES_PER_HOUR   (60)
#define HOURS_PER_DAY      (24)
#define DAYS_PER_WEEK      (7)
#define DAYS_PER_MONTH_MAX (31)
#define MONTHS_PER_YEAR    (12)
#define DAYS_PER_YEAR      (365)
#define DAYS_PER_LEAP_YEAR (366)
#define LEAP_YEAR_PERIOD   ((uint32_t)4)


uint8_t  clock_get_year();
uint8_t  clock_get_month();
uint8_t  clock_get_date();
uint8_t  clock_get_hour();
uint8_t  clock_get_minute();
uint8_t  clock_get_second();
bool     clock_save_time(const RTC_TimeTypeDef* time);
bool     clock_save_date(const RTC_DateTypeDef* date);
bool     clock_get_rtc_time(RTC_TimeTypeDef* time);
bool     clock_get_rtc_date(RTC_DateTypeDef* date);
uint32_t clock_datetime_to_seconds(const RTC_DateTypeDef* date, const RTC_TimeTypeDef* time);
uint32_t clock_get_timestamp();
void     clock_seconds_to_datetime(const uint32_t seconds, RTC_DateTypeDef* date, RTC_TimeTypeDef* time);


#ifdef __cplusplus
}
#endif


#endif
