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


#define MIN_TANK_VOLUME       3500
#define MAX_TANK_VOLUME       50
#define MAX_TANK_LTR          3500
#define SETTING_VALUE_MIN     0
#define MAX_ADC_VALUE         4095


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
settings_tag_t* settings_cbs[] = {
	&general_settings_load,
	NULL
};


void _general_settings_default(settings_sd_payload_t* payload) {
	LOG_DEBUG(SETTINGS_TAG, "SET DEFAULT SETTINGS\r\n");
	payload->v1.payload_settings.id = 1;
	memset(payload->v1.payload_settings.server_url, 0, sizeof(payload->v1.payload_settings.server_url));
	memset(payload->v1.payload_settings.server_port, 0, sizeof(payload->v1.payload_settings.server_port));
	strncpy(payload->v1.payload_settings.server_url, _default_server_url, strlen(_default_server_url));
	strncpy(payload->v1.payload_settings.server_port, _default_server_port ,strlen(_default_server_port));
	payload->v1.payload_settings.tank_liters_min = 0.0;
	payload->v1.payload_settings.tank_liters_max = MAX_TANK_LTR;
	payload->v1.payload_settings.tank_ADC_max = MAX_TANK_VOLUME;
	payload->v1.payload_settings.tank_ADC_min = MIN_TANK_VOLUME;
	payload->v1.payload_settings.milliliters_per_day = MAX_TANK_LTR;
	payload->v1.payload_settings.pump_speed = 0;
	payload->v1.payload_settings.sleep_time = DEFAULT_SLEEPING_TIME;
	payload->v1.payload_settings.module_enabled = true;
	payload->v1.payload_settings.server_log_id = 0;
	payload->v1.payload_settings.pump_work_seconds = 0;
	payload->v1.payload_settings.cf_id = CF_VERSION_DEFAULT;
	settings_save();
}


void _general_settings_save(settings_sd_payload_t* payload) {
	LOG_DEBUG(SETTINGS_TAG, "SAVE SETTINGS\r\n");
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
	payload->v1.payload_settings.module_enabled = module_settings.module_enabled;
	payload->v1.payload_settings.server_log_id = module_settings.server_log_id;
	payload->v1.payload_settings.pump_work_seconds = module_settings.pump_work_seconds;
	payload->v1.payload_settings.cf_id = module_settings.cf_id;
	show_settings();
}

void _general_settings_load(const settings_sd_payload_t* payload) {
	LOG_DEBUG(SETTINGS_TAG, "LOAD SETTINGS\r\n");
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
	module_settings.module_enabled = payload->v1.payload_settings.module_enabled;
	module_settings.server_log_id = payload->v1.payload_settings.server_log_id;
	module_settings.pump_work_seconds = payload->v1.payload_settings.pump_work_seconds;
	module_settings.cf_id = payload->v1.payload_settings.cf_id;
	show_settings();
}

void show_settings()
{
	LOG_DEBUG(
		SETTINGS_TAG,
		"\nTime: %u-%02u-%02uT%02u:%02u:%02u\r\n"
		"Device ID: %lu\n"
		"Server URL: %s:%s\n"
		"ADC level MIN: %u\n"
		"ADC level MAX: %u\n"
		"Liquid level MIN: %u l\n"
		"Liquid level MAX: %u l\n"
		"Target: %lu ml/d\n"
		"Pump speed: %lu ml/h\n"
		"Sleep time: %lu sec\n"
		"Server log ID: %lu\n"
		"Pump work: %lu sec\n"
		"Config ver: %d\n\n",
		DS1307_GetYear(),
		DS1307_GetMonth(),
		DS1307_GetDate(),
		DS1307_GetHour(),
		DS1307_GetMinute(),
		DS1307_GetSecond(),
		module_settings.id,
		module_settings.server_url,
		module_settings.server_port,
		module_settings.tank_ADC_max,
		module_settings.tank_ADC_min,
		module_settings.tank_liters_min,
		module_settings.tank_liters_max,
		module_settings.milliliters_per_day,
		module_settings.pump_speed,
		module_settings.sleep_time / MILLIS_IN_SECOND,
		module_settings.server_log_id,
		module_settings.pump_work_seconds,
		module_settings.cf_id
	);
}

int _write(int file, uint8_t *ptr, int len) {
	HAL_UART_Transmit(&COMMAND_UART, (uint8_t *) ptr, len, DEFAULT_UART_DELAY);
	for (int DataIdx = 0; DataIdx < len; DataIdx++) {
		ITM_SendChar(*ptr++);
	}
	return len;
}
