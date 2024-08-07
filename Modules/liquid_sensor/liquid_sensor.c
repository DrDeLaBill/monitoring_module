/*
 * liquid_sensor.c
 *
 *  Created on: 4 сент. 2022 г.
 *      Author: georg
 */

#include "liquid_sensor.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "glog.h"
#include "main.h"
#include "gutils.h"
#include "system.h"
#include "defines.h"
#include "settings.h"


#define LEVEL_LATENCY (10)


uint16_t _get_liquid_adc_value();
int32_t  _get_liquid_liters(uint32_t adc);
uint32_t _get_cur_liquid_adc();


const char* LIQUID_TAG = "LQID";


bool started = false;
unsigned counter = 0;
uint32_t level_adc[100] = {0};
util_old_timer_t timer = {0};


void level_tick()
{
	if (counter >= __arr_len(level_adc)) {
		started = true;
		counter = 0;
	}
	if (util_old_timer_wait(&timer)) {
		return;
	}
	util_old_timer_start(&timer, 100);
	level_adc[counter++] = _get_cur_liquid_adc();
}

int32_t get_level()
{
	if (!started) {
		return LEVEL_ERROR;
	}
	int32_t sum = 0;
	for (unsigned i = 0; i < __arr_len(level_adc); i++) {
		int32_t level = _get_liquid_liters(level_adc[i]);
		if (level == LEVEL_ERROR) {
			return LEVEL_ERROR;
		}
		sum += level;
	}
	return sum / (int32_t)__arr_len(level_adc);
}

uint32_t get_level_adc()
{
	if (!started) {
		return (uint16_t)settings.tank_ADC_min;
	}
	uint32_t sum = 0;
	for (unsigned i = 0; i < __arr_len(level_adc); i++) {
		sum += level_adc[i];
	}
	return sum / __arr_len(level_adc);
}

bool is_tank_empty()
{
	return _get_cur_liquid_adc() > settings.tank_ADC_min + LEVEL_LATENCY;
}

uint32_t _get_cur_liquid_adc()
{
	return SYSTEM_ADC_VOLTAGE[1];
}

int32_t _get_liquid_liters(uint32_t adc)
{
	if (adc >= MAX_ADC_VALUE) {
		printTagLog(LIQUID_TAG, "error liquid tank: get liquid ADC value - value more than MAX=%d (ADC=%u)\n", MAX_ADC_VALUE, adc);
		return LEVEL_ERROR;
	}

	if (adc > settings.tank_ADC_min + LEVEL_LATENCY ||
		adc + LEVEL_LATENCY < settings.tank_ADC_max
	) {
		printTagLog(LIQUID_TAG, "error liquid tank: settings error - ADC=%u, ADC_min=%lu, ADC_max=%lu\n", adc, settings.tank_ADC_min, settings.tank_ADC_max);
		return LEVEL_ERROR;
	}

	uint32_t adc_range = __abs_dif(settings.tank_ADC_min, settings.tank_ADC_max);
	uint32_t ltr_range = __abs_dif(settings.tank_ltr_max, settings.tank_ltr_min);
	if (adc_range == 0) {
		printTagLog(LIQUID_TAG, "error liquid tank: settings error - liters_range=%lu, ADC_range=%lu\n", ltr_range, adc_range);
		return LEVEL_ERROR;
	}

	uint32_t end = adc_range;
	if (adc > settings.tank_ADC_min) {
		return (int32_t)settings.tank_ltr_min;
	}
	if (adc < settings.tank_ADC_max) {
		return (int32_t)settings.tank_ltr_max;
	}
	if (end == 0) {
		printTagLog(LIQUID_TAG, "error liquid tank: settings error - ADC=%u, tank_ADC_min=%lu, tank_ADC_max=%lu\n", adc, settings.tank_ADC_min, settings.tank_ADC_max);
		return LEVEL_ERROR;
	}
	uint32_t value = 0;
	if (adc >= settings.tank_ADC_max) {
		value = adc - settings.tank_ADC_max;
	}
	value = end - value;

	int32_t ltr_res = (int32_t)(settings.tank_ltr_min + ((value * ltr_range) / end));
	if (ltr_res <= 0) {
		printTagLog(LIQUID_TAG, "error liquid tank: get liquid liters - value less or equal to zero (val=%ld)\n", ltr_res);
		return LEVEL_ERROR;
	}

	return ltr_res;
}
