/*
 * liquid_sensor.h
 *
 *  Created on: 4 сент. 2022 г.
 *      Author: DrDeLaBill
 */

#ifndef INC_LIQUID_SENSOR_H_
#define INC_LIQUID_SENSOR_H_


#ifdef __cplusplus
extern "C" {
#endif


#include "stm32f1xx_hal.h"
#include <stdbool.h>


#define LEVEL_ERROR (-1)


void     level_tick();
int32_t  get_level();
uint32_t get_level_adc();
bool     is_tank_empty();


#ifdef __cplusplus
}
#endif


#endif /* INC_LIQUID_SENSOR_H_ */
