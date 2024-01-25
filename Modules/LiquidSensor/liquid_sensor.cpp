/*
 * liquid_sensor.c
 *
 *  Created on: 4 сент. 2022 г.
 *      Author: georg
 */

#include "liquid_sensor.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "utils.h"
#include "main.h"

#include "SettingsDB.h"


#define LIQUID_ERROR         (-1)
#define LIQUID_ADC_CHANNEL   (0)
#define LIQUID_MEASURE_COUNT (10)
#define LIQUID_MEASURE_DELAY ((uint32_t)300)


typedef struct _sens_state_t {
	util_timer_t tim;
	int32_t      measures_val[LIQUID_MEASURE_COUNT];
	int32_t      measures_adc[LIQUID_MEASURE_COUNT];
	uint8_t      counter;
} sens_state_t;


uint16_t _get_liquid_adc_value();
int32_t  _get_liquid_liters();
uint16_t _get_cur_liquid_adc();
int32_t  _get_liquid_liters();


extern SettingsDB settings;


const char* LIQUID_TAG = "LQID";


sens_state_t sens_state = {
	.tim          = {},
	.measures_val = {},
	.measures_adc = {},
	.counter      = 0
};


void liquid_sensor_tick()
{
	if (util_is_timer_wait(&(sens_state.tim))) {
		return;
	}

	sens_state.measures_adc[sens_state.counter]   = _get_cur_liquid_adc();
	sens_state.measures_val[sens_state.counter++] = _get_liquid_liters();
	if (sens_state.counter >= __arr_len(sens_state.measures_val)) {
		sens_state.counter = 0;
	}
	util_timer_start(&(sens_state.tim), LIQUID_MEASURE_DELAY);
}

int32_t get_liquid_level()
{
	int32_t result = 0;
	for (unsigned i = 0; i < __arr_len(sens_state.measures_val); i++) {
		result += sens_state.measures_val[i];
	}
	return result / __arr_len(sens_state.measures_val);
}

uint16_t get_liquid_adc()
{
	uint16_t result = 0;
	for (unsigned i = 0; i < __arr_len(sens_state.measures_adc); i++) {
		result += sens_state.measures_adc[i];
	}
	return result / __arr_len(sens_state.measures_adc);
}

uint16_t _get_cur_liquid_adc() {
	ADC_ChannelConfTypeDef conf = {
		.Channel = LIQUID_ADC_CHANNEL,
		.Rank = 1,
		.SamplingTime = ADC_SAMPLETIME_28CYCLES_5,
	};
	if (HAL_ADC_ConfigChannel(&MEASURE_ADC, &conf) != HAL_OK) {
		return 0;
	}
	HAL_ADC_Start(&MEASURE_ADC);
	HAL_ADC_PollForConversion(&MEASURE_ADC, ADC_READ_TIMEOUT);
	uint16_t liquid_ADC_value = HAL_ADC_GetValue(&MEASURE_ADC);
	HAL_ADC_Stop(&MEASURE_ADC);
	return liquid_ADC_value;
}

int32_t _get_liquid_liters()
{
	uint16_t liquid_ADC_value = _get_cur_liquid_adc();
	if (liquid_ADC_value >= MAX_ADC_VALUE) {
		LOG_TAG_BEDUG(LIQUID_TAG, "error liquid tank: get liquid ADC value - value more than MAX=%d (ADC=%d)\n", MAX_ADC_VALUE, liquid_ADC_value);
		return LIQUID_ERROR;
	}

	if (liquid_ADC_value > settings.settings.tank_ADC_min || liquid_ADC_value < settings.settings.tank_ADC_max) {
		LOG_TAG_BEDUG(LIQUID_TAG, "error liquid tank: settings error - liquid_ADC_valu=%u, tank_ADC_min=%lu, tank_ADC_max=%lu\n", liquid_ADC_value, settings.settings.tank_ADC_min, settings.settings.tank_ADC_max);
		return LIQUID_ERROR;
	}

	uint32_t liquid_ADC_range = __abs_dif(settings.settings.tank_ADC_min, settings.settings.tank_ADC_max);
	uint32_t liquid_liters_range = __abs_dif(settings.settings.tank_liters_max, settings.settings.tank_liters_min) / MILLILITERS_IN_LITER;
	if (liquid_ADC_range == 0) {
		LOG_TAG_BEDUG(LIQUID_TAG, "error liquid tank: settings error - tank_liters_range=%lu, liquid_ADC_range=%lu\n", liquid_liters_range, liquid_ADC_range);
		return LIQUID_ERROR;
	}

	uint32_t min_in_liters = settings.settings.tank_liters_min / MILLILITERS_IN_LITER;
	int32_t liquid_in_liters = (liquid_liters_range - ((liquid_ADC_value * liquid_liters_range) / liquid_ADC_range)) + min_in_liters;
	if (liquid_in_liters <= 0) {
		LOG_TAG_BEDUG(LIQUID_TAG, "error liquid tank: get liquid liters - value less or equal to zero (val=%ld)\n", liquid_in_liters);
		return LIQUID_ERROR;
	}

	return liquid_in_liters;
}

bool is_liquid_tank_empty()
{
	return _get_cur_liquid_adc() > settings.settings.tank_ADC_min;
}
