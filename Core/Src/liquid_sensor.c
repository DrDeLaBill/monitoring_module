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
#include "settings.h"
#include "utils.h"


#define LIQUID_ERROR         -1
#define MILLILITERS_IN_LITER 1000


uint16_t _get_liquid_adc_value();
float _get_liquid_in_liters();


const char* LIQUID_TAG = "LQID";
const char* error = " SENSOR ERROR\n";


void liquid_sensor_begin()
{
	HAL_ADCEx_Calibration_Start(&LIQUID_ADC);
}

uint16_t get_liquid_adc() {
	HAL_ADC_Start(&LIQUID_ADC);
	HAL_ADC_PollForConversion(&LIQUID_ADC, READ_TIMEOUT);
	uint16_t liquid_ADC_value = HAL_ADC_GetValue(&LIQUID_ADC);
	HAL_ADC_Stop(&LIQUID_ADC);
	return liquid_ADC_value;
}

int32_t get_liquid_liters()
{
	uint16_t liquid_ADC_value = get_liquid_adc();
	if (liquid_ADC_value >= MAX_ADC_VALUE) {
		LOG_DEBUG(LIQUID_TAG, "%s\r\n", error);
		return LIQUID_ERROR;
	}
	uint16_t liquid_ADC_range = abs(module_settings.tank_ADC_min - module_settings.tank_ADC_max);
	uint16_t liquid_liters_range = abs(module_settings.tank_liters_max - module_settings.tank_liters_min);
	if (liquid_ADC_range == 0) {
		LOG_DEBUG(LIQUID_TAG, "ERROR MIN-MAX\n");
		return LIQUID_ERROR;
	}
	int32_t liquid_in_liters = (liquid_ADC_range - liquid_ADC_value) * liquid_liters_range / liquid_ADC_range + module_settings.tank_liters_min;
	if (liquid_in_liters <= 0) {
		LOG_DEBUG(LIQUID_TAG, error);
		return LIQUID_ERROR;
	}
	return liquid_in_liters;
}

bool is_liquid_tank_empty()
{
	return get_liquid_adc() > module_settings.tank_ADC_min;
}
