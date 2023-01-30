/*
 * pressure_sensor.h
 *
 *  Created on: Jan 30, 2023
 *      Author: georg
 */

#ifndef INC_PRESSURE_SENSOR_H_
#define INC_PRESSURE_SENSOR_H_

typedef struct _channel_measurement {
	float shunt_buf;
	float shunt_val;
} channel_measurement;


void void pressure_sensor_begin();
float get_first_press();
float get_second_press();
void pressure_sensor_proccess();


#endif /* INC_PRESSURE_SENSOR_H_ */
