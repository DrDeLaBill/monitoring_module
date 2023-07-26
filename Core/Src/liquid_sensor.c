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
#define MILLILITERS_IN_LITER 1000
#define LIQUID_ADC_CHANNEL   0


uint16_t _get_liquid_adc_value();
float _get_liquid_in_liters();


const char* LIQUID_TAG = "LQID";
const char* error = " SENSOR ERROR\n";

const uint16_t adc_ranges[] = {2370, 2318, 2203, 2045, 1832, 1541, 1068, 175, 59 };
const uint16_t val_ranges[] = {10,   45,   90,   150,  200,  250,  300,  350, 375};


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
	if (sizeof(adc_ranges) / sizeof(*adc_ranges) < 2 || sizeof(val_ranges) / sizeof(*val_ranges) < 2) {
		return LIQUID_ERROR;
	}

	uint16_t liquid_ADC_value = get_liquid_adc();
	if (liquid_ADC_value >= MAX_ADC_VALUE) {
		LOG_DEBUG(LIQUID_TAG, "%s\r\n", error);
		return LIQUID_ERROR;
	}

	int32_t converted_val = 0;
	for (int i = 0; i < (sizeof(adc_ranges) / sizeof(*adc_ranges)) - 1; i++) {
		if (liquid_ADC_value < adc_ranges[i] && liquid_ADC_value > adc_ranges[i + 1]) {
			uint32_t cur_adc_range = __abs(adc_ranges[i] - adc_ranges[i + 1]);
			uint32_t cur_val_range = __abs(val_ranges[i + 1] - val_ranges[i]);
			uint32_t cur_adc_delta = __abs(adc_ranges[i] - liquid_ADC_value);
			uint32_t cur_val_delta = (cur_adc_delta * cur_val_range) / cur_adc_range;
			converted_val = val_ranges[i] + cur_val_delta;
			break;
		}
	}
	if (liquid_ADC_value < adc_ranges[sizeof(adc_ranges) / sizeof(*adc_ranges) - 1]) {
		converted_val = val_ranges[sizeof(val_ranges) / sizeof(*val_ranges) - 1];
	}

	if (converted_val <= 0) {
		LOG_DEBUG(LIQUID_TAG, error);
		return LIQUID_ERROR;
	}
	return converted_val;
}

bool is_liquid_tank_empty()
{
	return get_liquid_adc() > module_settings.tank_ADC_min;
}
