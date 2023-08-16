/* Copyright © 2023 Georgy E. All rights reserved. */

#include "pressure_sensor.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "utils.h"
#include "defines.h"

#define PRESS_MPA_x100_MAX ((uint16_t)1600)
#define PRESS_MPA_x100_MIN ((uint16_t)0)
#define PRESS_ADC_VAL_MIN  ((uint16_t)780)
#define PRESS_ADC_VAL_MAX  ((uint16_t)3916)
#define PRESS_ADC_CHANNELL ((uint32_t)5)
#define PRESS_WAIT_TIME_MS ((uint16_t)100)


const char* PRESS_TAG = "PRES:";

press_measure_t press_measure = {
	.measure_ready      = false,
	.value              = 0,
	.measure_values_idx = 0,
	.measure_values     = {0},
	.wait_timer         = {0}
};


uint16_t _pressure_get_adc_value();


void pressure_sensor_proccess()
{
	if (util_is_timer_wait(&press_measure.wait_timer)) {
		return;
	}

	util_timer_start(&press_measure.wait_timer, PRESS_WAIT_TIME_MS);

	uint8_t measure_values_len = sizeof(press_measure.measure_values) / sizeof(*press_measure.measure_values);

	if (press_measure.measure_values_idx < measure_values_len) {
		press_measure.measure_values[press_measure.measure_values_idx++] = _pressure_get_adc_value();
		return;
	}

	press_measure.measure_ready = true;
	press_measure.measure_values_idx = 0;

	uint32_t measure_sum = 0;
	for (uint8_t i = 0; i < measure_values_len; i++) {
		measure_sum += press_measure.measure_values[i];
	}

	uint32_t adc_value = measure_sum / measure_values_len;

	if (adc_value < PRESS_ADC_VAL_MIN) {
		press_measure.value = 0;
		return;
	}

	press_measure.value = util_convert_range(adc_value, PRESS_ADC_VAL_MIN, PRESS_ADC_VAL_MAX, PRESS_MPA_x100_MIN, PRESS_MPA_x100_MAX);
}

uint32_t get_press()
{
	if (!press_measure.measure_ready) {
		return 0;
	}

	return press_measure.value;
}

uint16_t _pressure_get_adc_value()
{
	ADC_ChannelConfTypeDef conf = {
		.Channel = PRESS_ADC_CHANNELL,
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

