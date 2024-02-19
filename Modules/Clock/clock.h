/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _CLOCK_H_
#define _CLOCK_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#include "hal_defs.h"

/**
  * @brief  RTC Time structure definition
  */
typedef struct
{
  uint8_t Hours;            /*!< Specifies the RTC Time Hour.
                                 This parameter must be a number between Min_Data = 0 and Max_Data = 12 if the HourFormat_12 is selected
                                 This parameter must be a number between Min_Data = 0 and Max_Data = 23 if the HourFormat_24 is selected */

  uint8_t Minutes;          /*!< Specifies the RTC Time Minutes.
                                 This parameter must be a number between Min_Data = 0 and Max_Data = 59 */

  uint8_t Seconds;          /*!< Specifies the RTC Time Seconds.
                                 This parameter must be a number between Min_Data = 0 and Max_Data = 59 */

  uint8_t TimeFormat;       /*!< Specifies the RTC AM/PM Time.
                                 This parameter can be a value of @ref AM_PM_Definitions */

  uint32_t SubSeconds;      /*!< Specifies the SSR RTC Sub Second register content.
                                 This parameter corresponds to a time unit range between [0-1] Second
                                 with [1 Sec / SecondFraction +1] granularity */

  uint32_t SecondFraction;  /*!< Specifies the range or granularity of Sub Second register content
                                 corresponding to Synchronous prescaler factor value (PREDIV_S)
                                 This parameter corresponds to a time unit range between [0-1] Second
                                 with [1 Sec / SecondFraction +1] granularity.
                                 This field will be used only by HAL_GetTime function */

  uint32_t DayLightSaving;  /*!< This interface is deprecated. To manage Daylight
                                 Saving Time, please use HAL_DST_xxx functions */

  uint32_t StoreOperation;  /*!< This interface is deprecated. To manage Daylight
                                 Saving Time, please use HAL_DST_xxx functions */
} TimeTypeDef;

/**
  * @brief  RTC Date structure definition
  */
typedef struct
{
  uint8_t WeekDay;  /*!< Specifies the RTC Date WeekDay.
                         This parameter can be a value of @ref WeekDay_Definitions */

  uint8_t Month;    /*!< Specifies the RTC Date Month (in BCD format).
                         This parameter can be a value of @ref Month_Date_Definitions */

  uint8_t Date;     /*!< Specifies the RTC Date.
                         This parameter must be a number between Min_Data = 1 and Max_Data = 31 */

  uint8_t Year;     /*!< Specifies the RTC Date Year.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 99 */

} DateTypeDef;


uint8_t  clock_get_year();
uint8_t  clock_get_month();
uint8_t  clock_get_date();
uint8_t  clock_get_hour();
uint8_t  clock_get_minute();
uint8_t  clock_get_second();
void     clock_save_time(TimeTypeDef* time);
void     clock_save_date(DateTypeDef* date);
void     clock_get_rtc_time(TimeTypeDef* time);
void     clock_get_rtc_date(DateTypeDef* date);
uint32_t clock_datetime_to_seconds(DateTypeDef* date, TimeTypeDef* time);
uint32_t clock_get_timestamp();


#ifdef __cplusplus
}
#endif


#endif
