/*
 * liquid_sensor.c
 *
 *  Created on: 4 сент. 2022 г.
 *      Author: georg
 */

#include "liquid_sensor.h"

#include <stdbool.h>
#include <stdlib.h>

#include "defines.h"
#include "utils.h"
#include "main.h"
#include "settings_manager.h"


#define LIQUID_ERROR         -1
#define LIQUID_ADC_CHANNEL   0


uint16_t _get_liquid_adc_value();
float _get_liquid_in_liters();


const char* LIQUID_TAG = "LQID";


uint16_t get_liquid_adc() {
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

int32_t get_liquid_liters()
{
	uint16_t liquid_ADC_value = get_liquid_adc();
	if (liquid_ADC_value >= MAX_ADC_VALUE) {
		LOG_TAG_BEDUG(LIQUID_TAG, "error liquid tank: get liquid ADC value - value more than MAX=%d (ADC=%d)\n", MAX_ADC_VALUE, liquid_ADC_value);
		return LIQUID_ERROR;
	}

	if (liquid_ADC_value > module_settings.tank_ADC_min || liquid_ADC_value < module_settings.tank_ADC_max) {
		LOG_TAG_BEDUG(LIQUID_TAG, "error liquid tank: settings error - liquid_ADC_valu=%u, tank_ADC_min=%lu, tank_ADC_max=%lu\n", liquid_ADC_value, module_settings.tank_ADC_min, module_settings.tank_ADC_max);
		return LIQUID_ERROR;
	}

	uint32_t liquid_ADC_range = __abs(module_settings.tank_ADC_min - module_settings.tank_ADC_max);
	uint32_t liquid_liters_range = __abs(module_settings.tank_liters_max - module_settings.tank_liters_min) / MILLILITERS_IN_LITER;
	if (liquid_ADC_range == 0) {
		LOG_TAG_BEDUG(LIQUID_TAG, "error liquid tank: settings error - tank_liters_range=%lu, liquid_ADC_range=%lu\n", liquid_liters_range, liquid_ADC_range);
		return LIQUID_ERROR;
	}

	uint32_t min_in_liters = module_settings.tank_liters_min / MILLILITERS_IN_LITER;
	int32_t liquid_in_liters = (liquid_liters_range - ((liquid_ADC_value * liquid_liters_range) / liquid_ADC_range)) + min_in_liters;
	if (liquid_in_liters <= 0) {
		LOG_TAG_BEDUG(LIQUID_TAG, "error liquid tank: get liquid liters - value less or equal to zero (val=%ld)\n", liquid_in_liters);
		return LIQUID_ERROR;
	}

	return liquid_in_liters;
}

bool is_liquid_tank_empty()
{
	return get_liquid_adc() > module_settings.tank_ADC_min;
}
