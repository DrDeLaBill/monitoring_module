/*
 * pressure_sensor.c
 *
 *  Created on: Jan 30, 2023
 *      Author: georg
 */


#include "pressure_sensor.h"

#include <string.h>

#include "main.h"
#include "utils.h"
#include "defines.h"


#define PRESS_ADC_VAL_MIN     780
#define PRESS_ADC_VAL_MAX     3916
#define CURRENT_MIN           400
#define CURRENT_MAX           2000
#define CHANNELS_COUNT        1
#define FIRST_SHUNT_NUM       0
#define SECOND_SHUNT_NUM      1
#define SHUNT_ERROR_VAL       0
#define PRESSURE_ADC_CHANNEL1 1


channel_measurement measurement_buf[CHANNELS_COUNT] = {};

const char* PRESS_TAG = "PRES:";


void _do_channel_measurements(uint8_t channel_num);
uint16_t _pressure_get_adc_value();
uint32_t _pressure_get_absolute_value();


void pressure_sensor_begin()
{
	memset((uint8_t*)&measurement_buf, 0, sizeof(measurement_buf));
	for (uint8_t i = 0; i < sizeof(measurement_buf) / sizeof(channel_measurement); i++) {
		measurement_buf[i].state = TO_MIN;
	}
}

uint32_t get_first_press()
{
	return measurement_buf[0].shunt_val_max;
}

//uint32_t get_second_press()
//{
//	return measurement_buf[1].shunt_val_max;
//}

void pressure_sensor_proccess()
{
	_do_channel_measurements(FIRST_SHUNT_NUM);
}

void _do_channel_measurements(uint8_t channel_num)
{
	if (channel_num >= sizeof(measurement_buf) / sizeof(channel_measurement)) {
		return;
	}

	channel_measurement *channel_ms = &measurement_buf[channel_num];
	uint32_t cur_level = _pressure_get_absolute_value();

	if (cur_level <= CURRENT_MIN) {
		channel_ms->shunt_buf_max = 0.0;
		channel_ms->shunt_buf_min = CURRENT_MAX;
		channel_ms->shunt_val_max = SHUNT_ERROR_VAL;
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

uint16_t _pressure_get_adc_value()
{
	ADC_ChannelConfTypeDef conf = {
		.Channel = PRESSURE_ADC_CHANNEL1,
		.Rank = 1,
		.SamplingTime = ADC_SAMPLETIME_28CYCLES_5,
	};
	if (HAL_ADC_ConfigChannel(&MEASURE_ADC, &conf) != HAL_OK) {
		return 0;
	}
	HAL_ADC_Start(&MEASURE_ADC);
	HAL_ADC_PollForConversion(&MEASURE_ADC, ADC_READ_TIMEOUT);
	uint16_t pressure_ADC_value = HAL_ADC_GetValue(&MEASURE_ADC);
	HAL_ADC_Stop(&MEASURE_ADC);
	return pressure_ADC_value;
}

uint32_t _pressure_get_absolute_value()
{
	return util_convert_range(_pressure_get_adc_value(), PRESS_ADC_VAL_MIN, PRESS_ADC_VAL_MAX, CURRENT_MIN, CURRENT_MAX);
}

