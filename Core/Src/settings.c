/*
 * functions.h
 *
 *  Created on: Sep 4, 2022
 *      Author: georg
 */

#include "settings.h"

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"
#include "settings_manager.h"

// Clock
#include "ds1307_for_stm32_hal.h"

void _general_settings_default(settings_sd_payload_t* payload);
void _general_settings_save(settings_sd_payload_t* payload);
void _general_settings_load(const settings_sd_payload_t* payload);

const char *SETTINGS_TAG = "STNGS";
const char *_default_server_url = "192.168.0.1";
const char *_default_server_port = "80";

ModuleSettings module_settings;
settings_tag_t general_settings_load = {
	.default_cb = &_general_settings_default,
	.save_cb = &_general_settings_save,
	.load_cb = &_general_settings_load
};

void _general_settings_default(settings_sd_payload_t* payload) {
	LOG_DEBUG(SETTINGS_TAG, "%s\r\n", "SET DEFAULT SETTINGS");
	payload->v1.payload_settings.id = 1;
	memset(payload->v1.payload_settings.server_url, 0, sizeof(payload->v1.payload_settings.server_url));
	memset(payload->v1.payload_settings.server_port, 0, sizeof(payload->v1.payload_settings.server_port));
	strncpy(payload->v1.payload_settings.server_url, _default_server_url, strlen(_default_server_url));
	strncpy(payload->v1.payload_settings.server_port, _default_server_port ,strlen(_default_server_port));
	payload->v1.payload_settings.tank_liters_min = 0.0;
	payload->v1.payload_settings.tank_liters_max = MAX_TANK_VOLUME;
	payload->v1.payload_settings.tank_ADC_max = 0;
	payload->v1.payload_settings.tank_ADC_min = MAX_TANK_VOLUME;
	payload->v1.payload_settings.milliliters_per_day = MAX_TANK_VOLUME;
	payload->v1.payload_settings.pump_speed = 0;
	payload->v1.payload_settings.sleep_time = DEFAULT_SLEEPING_TIME;
	payload->v1.payload_settings.clock_initialized = false;
	show_settings();
}


void _general_settings_save(settings_sd_payload_t* payload) {
	payload->v1.payload_settings.id = module_settings.id;
	strncpy(payload->v1.payload_settings.server_url, module_settings.server_url, strlen(module_settings.server_url));
	strncpy(payload->v1.payload_settings.server_port, module_settings.server_port, strlen(module_settings.server_port));
	payload->v1.payload_settings.tank_liters_min = module_settings.tank_liters_min;
	payload->v1.payload_settings.tank_liters_max = module_settings.tank_liters_max;
	payload->v1.payload_settings.tank_ADC_max = module_settings.tank_ADC_max;
	payload->v1.payload_settings.tank_ADC_min = module_settings.tank_ADC_min;
	payload->v1.payload_settings.milliliters_per_day = module_settings.milliliters_per_day;
	payload->v1.payload_settings.pump_speed = module_settings.pump_speed;
	payload->v1.payload_settings.sleep_time = module_settings.sleep_time;
	payload->v1.payload_settings.clock_initialized = module_settings.clock_initialized;
	show_settings();
}

void _general_settings_load(const settings_sd_payload_t* payload) {
	module_settings.id = payload->v1.payload_settings.id;
	strncpy(module_settings.server_url, payload->v1.payload_settings.server_url, strlen(payload->v1.payload_settings.server_url));
	strncpy(module_settings.server_port, payload->v1.payload_settings.server_port, strlen(payload->v1.payload_settings.server_port));
	module_settings.tank_liters_min = payload->v1.payload_settings.tank_liters_min;
	module_settings.tank_liters_max = payload->v1.payload_settings.tank_liters_max;
	module_settings.tank_ADC_max = payload->v1.payload_settings.tank_ADC_max;
	module_settings.tank_ADC_min = payload->v1.payload_settings.tank_ADC_min;
	module_settings.milliliters_per_day = payload->v1.payload_settings.milliliters_per_day;
	module_settings.pump_speed = payload->v1.payload_settings.pump_speed;
	module_settings.sleep_time = payload->v1.payload_settings.sleep_time;
	module_settings.clock_initialized = payload->v1.payload_settings.clock_initialized;
	show_settings();
}

bool set_clock_initialized(bool value)
{
	module_settings.clock_initialized = value;
//	general_settings_load.save_cb(module_settings);
}

void show_settings()
{
	LOG_DEBUG(
		SETTINGS_TAG,
		"\r\nTime: %d-%02d-%02dT%02d:%02d:%02d\r\n"
		"Device ID: %d\r\n"
		"Server URL: %s:%s\r\n"
		"ADC level MIN: %d\r\n"
		"ADC level MAX: %d\r\n"
		"Liquid level MIN: %d l\r\n"
		"Liquid level MAX: %d l\r\n"
		"Target: %d ml/d\r\n"
		"Pump speed: %d ml/h\r\n"
		"Sleep time: %d sec\r\n\r\n",
		DS1307_GetYear(),
		DS1307_GetMonth(),
		DS1307_GetDate(),
		DS1307_GetHour(),
		DS1307_GetMinute(),
		DS1307_GetSecond(),
		module_settings.id,
		module_settings.server_url,
		module_settings.server_port,
		module_settings.tank_ADC_min,
		module_settings.tank_ADC_max,
		module_settings.tank_liters_min,
		module_settings.tank_liters_max,
		module_settings.milliliters_per_day,
		module_settings.pump_speed,
		module_settings.sleep_time / MILLIS_IN_SECOND
	);
}

int _write(int file, uint8_t *ptr, int len) {
	for (int DataIdx = 0; DataIdx < len; DataIdx++) {
		ITM_SendChar(*ptr++);
	}
	return len;
}
