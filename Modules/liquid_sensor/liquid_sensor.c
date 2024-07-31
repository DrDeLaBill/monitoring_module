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

#include "glog.h"
#include "main.h"
#include "gutils.h"
#include "system.h"
#include "defines.h"
#include "settings.h"


#define LEVEL_ERROR   (-1)
#define LEVEL_LATENCY (10)


uint16_t _get_liquid_adc_value();
int32_t  _get_liquid_liters();
uint32_t _get_cur_liquid_adc();
int32_t  _get_liquid_liters();


const char* LIQUID_TAG = "LQID";


int32_t get_liquid_level()
{
	return _get_liquid_liters();
}

uint16_t get_liquid_adc()
{
	return _get_cur_liquid_adc();
}

bool is_liquid_tank_empty()
{
	return _get_cur_liquid_adc() > settings.tank_ADC_min;
}

uint32_t _get_cur_liquid_adc() {
	return SYSTEM_ADC_VOLTAGE[1];
}

int32_t _get_liquid_liters()
{
	uint32_t liquid_ADC_value = _get_cur_liquid_adc();
	if (liquid_ADC_value >= MAX_ADC_VALUE) {
		printTagLog(LIQUID_TAG, "error liquid tank: get liquid ADC value - value more than MAX=%d (ADC=%d)\n", MAX_ADC_VALUE, liquid_ADC_value);
		return LEVEL_ERROR;
	}

	if (liquid_ADC_value > settings.tank_ADC_min + LEVEL_LATENCY ||
		liquid_ADC_value + LEVEL_LATENCY < settings.tank_ADC_max
	) {
		printTagLog(LIQUID_TAG, "error liquid tank: settings error - liquid_ADC_valu=%u, tank_ADC_min=%lu, tank_ADC_max=%lu\n", liquid_ADC_value, settings.tank_ADC_min, settings.tank_ADC_max);
		return LEVEL_ERROR;
	}

	uint32_t adc_range = __abs_dif(settings.tank_ADC_min, settings.tank_ADC_max);
	uint32_t ltr_range = __abs_dif(settings.tank_liters_max, settings.tank_liters_min);
	if (adc_range == 0) {
		printTagLog(LIQUID_TAG, "error liquid tank: settings error - tank_liters_range=%lu, liquid_ADC_range=%lu\n", ltr_range, adc_range);
		return LEVEL_ERROR;
	}

	uint32_t end = adc_range;
	if (liquid_ADC_value > settings.tank_ADC_min) {
		return (int32_t)settings.tank_liters_min;
	}
	if (liquid_ADC_value < settings.tank_ADC_max) {
		return (int32_t)settings.tank_liters_max;
	}
	if (end == 0) {
		printTagLog(LIQUID_TAG, "error liquid tank: settings error - liquid_ADC_valu=%u, tank_ADC_min=%lu, tank_ADC_max=%lu\n", liquid_ADC_value, settings.tank_ADC_min, settings.tank_ADC_max);
		return LEVEL_ERROR;
	}
	uint32_t value = liquid_ADC_value - settings.tank_ADC_max;
	value = end - value;

	int32_t ltr_res = (int32_t)(settings.tank_liters_min + ((value * ltr_range) / end)) / MILLILITERS_IN_LITER;
	if (ltr_res <= 0) {
		printTagLog(LIQUID_TAG, "error liquid tank: get liquid liters - value less or equal to zero (val=%ld)\n", ltr_res);
		return LEVEL_ERROR;
	}

	return ltr_res;
}
