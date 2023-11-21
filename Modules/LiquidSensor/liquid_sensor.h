/*
 * liquid_sensor.h
 *
 *  Created on: 4 сент. 2022 г.
 *      Author: DrDeLaBill
 */

#ifndef INC_LIQUID_SENSOR_H_
#define INC_LIQUID_SENSOR_H_

#include "stm32f1xx_hal.h"
#include <stdbool.h>

uint16_t get_liquid_adc();
int32_t get_liquid_liters();
bool is_liquid_tank_empty();

#endif /* INC_LIQUID_SENSOR_H_ */
