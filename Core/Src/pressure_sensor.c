/*
 * pressure_sensor.c
 *
 *  Created on: Jan 30, 2023
 *      Author: georg
 */


#include "pressure_sensor.h"

#include "ina3221_sensor.h"
#include "utils.h"


#define CURRENT_MIN      4.0
#define CURRENT_MAX      20.0
#define CHANNELS_COUNT   2
#define FIRST_SHUNT_NUM  0
#define SECOND_SHUNT_NUM 1


channel_measurement measurement_buf[CHANNELS_COUNT] = {};

const char* PRESS_TAG = "PRES:";


void _do_channel_measurements(uint8_t channel_num);


void pressure_sensor_begin()
{
	INA3221_begin();
	measurement_buf[0].shunt_buf_min = CURRENT_MAX;
	measurement_buf[0].shunt_buf_max = 0.0;
	measurement_buf[0].state = TO_MIN;
	measurement_buf[1].shunt_buf_min = CURRENT_MAX;
	measurement_buf[1].shunt_buf_max = 0.0;
	measurement_buf[1].state = TO_MIN;
}

float get_first_press()
{
	return measurement_buf[0].shunt_val_max;
}

float get_second_press()
{
	return measurement_buf[1].shunt_val_max;
}

void pressure_sensor_proccess()
{
	_do_channel_measurements(FIRST_SHUNT_NUM);
	_do_channel_measurements(SECOND_SHUNT_NUM);
//	LOG_DEBUG(PRESS_TAG, " time: %lu 1 - %d.%d, 2 - %d.%d\n", HAL_GetTick(), FLOAT_AS_STRINGS(measurement_buf[0].shunt_buf), FLOAT_AS_STRINGS(measurement_buf[1].shunt_buf));
}

void _do_channel_measurements(uint8_t channel_num)
{
	if (channel_num >= sizeof(measurement_buf) / sizeof(channel_measurement)) {
		return;
	}

	channel_measurement *channel_ms = &measurement_buf[channel_num];
	float cur_level = INA3221_getCurrent_mA(channel_num + 1);

	if (cur_level <= 0.0) {
		channel_ms->shunt_buf_max = 0.0;
		channel_ms->shunt_buf_min = CURRENT_MAX;
		channel_ms->shunt_val_max = INA3221_ERROR;
		channel_ms->state = TO_MIN;
		return;
	}

	if (channel_ms->shunt_buf_min < cur_level && channel_ms->state == TO_MIN) {
		channel_ms->shunt_buf_max = 0.0;
		channel_ms->state = TO_MAX;
	}

	if (channel_ms->shunt_buf_max > cur_level && channel_ms->state == TO_MAX) {
		channel_ms->shunt_buf_min = CURRENT_MAX;
		channel_ms->state = TO_MIN;
	}

	if (channel_ms->shunt_buf_max < cur_level) {
		channel_ms->shunt_buf_max = cur_level;
	}

	if (channel_ms->shunt_buf_min > cur_level) {
		channel_ms->shunt_buf_min = cur_level;
	}

	if (channel_ms->state == TO_MIN) {
		channel_ms->shunt_val_max = channel_ms->shunt_buf_max;
	}
}
