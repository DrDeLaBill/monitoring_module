/*
 * liquid_sensor.h
 *
 *  Created on: 4 сент. 2022 г.
 *      Author: georg
 */

#ifndef INC_LIQUID_SENSOR_H_
#define INC_LIQUID_SENSOR_H_

#include "stm32f1xx_hal.h"
#include <stdbool.h>

#define LOW_VOLTAGE 0
#define HIGH_VOLTAGE 3.3
#define MAX_ADC_VALUE 4095
#define READ_TIMEOUT 100

void liquid_sensor_begin();
uint16_t get_liquid_adc();
int32_t get_liquid_liters();

#endif /* INC_LIQUID_SENSOR_H_ */
