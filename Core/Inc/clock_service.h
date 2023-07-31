/*
 * clock.h
 *
 *  Created on: Jul 27, 2023
 *      Author: DrDeLaBill
 */

#ifndef INC_CLOCK_SERVICE_H_
#define INC_CLOCK_SERVICE_H_


#include <stdint.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"


uint8_t get_year();
uint8_t get_month();
uint8_t get_date();
uint8_t get_hour();
uint8_t get_minute();
uint8_t get_second();
bool save_date(RTC_DateTypeDef* date);
bool save_time(RTC_TimeTypeDef* time);


#endif /* INC_CLOCK_SERVICE_H_ */
