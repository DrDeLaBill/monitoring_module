/*
 * pressure_sensor.h
 *
 *  Created on: Jan 30, 2023
 *      Author: georg
 */

#ifndef INC_PRESSURE_SENSOR_H_
#define INC_PRESSURE_SENSOR_H_


#include "stm32f1xx_hal.h"


enum {
	TO_MAX = 1,
	TO_MIN
};

typedef struct _channel_measurement {
	float shunt_buf_min;
	float shunt_buf_max;
	float shunt_val_max;
	uint8_t state;
} channel_measurement;


void pressure_sensor_begin();
float get_first_press();
float get_second_press();
void pressure_sensor_proccess();


#endif /* INC_PRESSURE_SENSOR_H_ */
