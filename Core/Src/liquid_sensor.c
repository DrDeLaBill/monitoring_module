/*
 * liquid_sensor.c
 *
 *  Created on: 4 сент. 2022 г.
 *      Author: georg
 */

#include "liquid_sensor.h"

#include "defines.h"
#include "settings.h"
#include "utils.h"


const char* LIQUID_TAG = "LQID";
const char* error = "SENSOR ERROR";

void liquid_sensor_begin()
{
	HAL_ADCEx_Calibration_Start(&LIQUID_ADC);
}

uint16_t get_liquid_ADC_value() {
	HAL_ADC_Start(&LIQUID_ADC);
	HAL_ADC_PollForConversion(&LIQUID_ADC, READ_TIMEOUT);
	uint16_t liquid_ADC_value = HAL_ADC_GetValue(&LIQUID_ADC);
	HAL_ADC_Stop(&LIQUID_ADC);
	return liquid_ADC_value;
}

float get_liquid_in_liters()
{
	uint16_t liquid_ADC_value = get_liquid_ADC_value();
	if (liquid_ADC_value > MAX_ADC_VALUE) {
		LOG_DEBUG(LIQUID_TAG, "%s\r\n", error);
		return LIQUID_ERROR;
	}
	if (liquid_ADC_value > module_settings.tank_ADC_min || liquid_ADC_value < module_settings.tank_ADC_max) {
		LOG_DEBUG(
			LIQUID_TAG,
			"MAX: %d\r\n"
			"MIN: %d\r\n"
			"ADC: %d\r\n",
			module_settings.tank_ADC_max,
			module_settings.tank_ADC_min,
			liquid_ADC_value
		);
		return LIQUID_ERROR;
	}
	uint16_t liquid_ADC_range = abs(module_settings.tank_ADC_min - module_settings.tank_ADC_max);
	float liquid_liters_range = abs(module_settings.tank_liters_max - module_settings.tank_liters_min);
	if (liquid_ADC_range == 0) {
		LOG_DEBUG(LIQUID_TAG, "ERROR MIN-MAX");
		return LIQUID_ERROR;
	}
	float liquid_in_liters = 1.0 * (liquid_ADC_range - liquid_ADC_value) * liquid_liters_range / liquid_ADC_range + module_settings.tank_liters_min;
	if (liquid_in_liters < 0.0) {
		LOG_DEBUG(LIQUID_TAG, error);
		return LIQUID_ERROR;
	}
	return liquid_in_liters;
}
