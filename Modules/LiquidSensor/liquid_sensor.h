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

void     level_tick();
int32_t  get_liquid_level();
uint16_t get_liquid_adc();
bool     is_liquid_tank_empty();


#ifdef __cplusplus
}
#endif


#endif /* INC_LIQUID_SENSOR_H_ */
