/*
 * pressure_sensor.h
 *
 *  Created on: Jan 30, 2023
 *      Author: DrDeLaBill
 */

#ifndef INC_PRESSURE_SENSOR_H_
#define INC_PRESSURE_SENSOR_H_


#include "stm32f1xx_hal.h"


typedef enum _measurement_direction_t {
	TO_MAX = 1,
	TO_MIN
} measurement_direction_t;


typedef struct _channel_measurement {
	uint32_t shunt_buf_min;
	uint32_t shunt_buf_max;
	uint32_t shunt_val_max;
	measurement_direction_t  state;
} channel_measurement;


void pressure_sensor_begin();
uint32_t get_first_press();
//uint32_t get_second_press();
void pressure_sensor_proccess();


#endif /* INC_PRESSURE_SENSOR_H_ */
