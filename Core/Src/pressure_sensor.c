/*
 * pressure_sensor.c
 *
 *  Created on: Jan 30, 2023
 *      Author: georg
 */


#include "pressure_sensor.h"

#include "ina3221_sensor.h"
#include "utils.h"


#define ABS_VAL_MIN    50
#define CHANNELS_COUNT 2


channel_measurement measurement_buf[CHANNELS_COUNT] = {};

const char* PRESS_TAG = "PRES:";


void _do_channel_measurements(uint8_t channel_num);


void pressure_sensor_begin()
{
	INA3221_begin();
}

float get_first_press()
{
	return measurement_buf[0].shunt_val;
}

float get_second_press()
{
	return measurement_buf[1].shunt_val;
}

void pressure_sensor_proccess()
{
	_do_channel_measurements(0);
	_do_channel_measurements(1);
//	LOG_DEBUG(PRESS_TAG, " 1 - %d.%d, 2 - %d.%d\n", FLOAT_AS_STRINGS(measurement_buf[0].shunt_buf), FLOAT_AS_STRINGS(measurement_buf[1].shunt_buf));
}

void _do_channel_measurements(uint8_t channel_num)
{
	if (channel_num >= sizeof(measurement_buf)) {
		return;
	}
	if (INA3221_getShuntVoltage_mV(channel_num + 1) <= ABS_VAL_MIN) {
		measurement_buf[channel_num].shunt_buf = 0.0;
	}
	float cur_level = INA3221_getCurrent_mA(channel_num + 1);
	if (measurement_buf[channel_num].shunt_buf < cur_level) {
		measurement_buf[channel_num].shunt_buf = cur_level;
	}
	if (measurement_buf[channel_num].shunt_buf >= cur_level) {
		measurement_buf[channel_num].shunt_val = measurement_buf[channel_num].shunt_buf;
	}
}
