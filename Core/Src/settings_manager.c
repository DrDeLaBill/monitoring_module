#include "settings_manager.h"


#include <string.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

#include "utils.h"
#include "defines.h"
#include "storage_data_manager.h"
#include "ds1307_for_stm32_hal.h"


const char* STNG_MODULE_TAG = "STNG";

const char *_default_server_url = "urv.a.izhpt.com";
const char *_default_server_port = "80";


module_settings_t module_settings;


bool _settings_check(module_settings_t* sttngs);


settings_status_t settings_reset() {
	LOG_DEBUG(STNG_MODULE_TAG, "SET DEFAULT SETTINGSn");
	module_settings.id = 1;
	memset(module_settings.server_url, 0, sizeof(module_settings.server_url));
	memset(module_settings.server_port, 0, sizeof(module_settings.server_port));
	strncpy(module_settings.server_url, _default_server_url, strlen(_default_server_url));
	strncpy(module_settings.server_port, _default_server_port ,strlen(_default_server_port));
	module_settings.tank_liters_min = MIN_TANK_LTR;
	module_settings.tank_liters_max = MAX_TANK_LTR;
	module_settings.tank_ADC_max = MAX_TANK_VOLUME;
	module_settings.tank_ADC_min = MIN_TANK_VOLUME;
	module_settings.milliliters_per_day = MAX_TANK_LTR;
	module_settings.pump_speed = 0;
	module_settings.sleep_time = DEFAULT_SLEEPING_TIME;
	module_settings.pump_enabled = true;
	module_settings.server_log_id = 0;
	module_settings.cf_id = CF_VERSION_DEFAULT;
	module_settings.pump_work_day_sec = 0;
	module_settings.pump_downtime_sec = 0;
	module_settings.pump_work_sec = 0;
	module_settings.pump_log_date = 0;
	module_settings.is_first_start = 1;
	return settings_save();
}


settings_status_t settings_load() {
	module_settings_t buff;
	memset(&buff, 0, sizeof(buff));

	uint32_t stor_addr = 0;
	storage_status_t status = storage_get_first_available_addr(&stor_addr);
	if (status != STORAGE_OK) {
#ifdef DEBUG
		LOG_DEBUG(STNG_MODULE_TAG, " load settings: error storage access\n");
#endif
		return SETTINGS_ERROR;
	}

	status = storage_load(stor_addr, (uint8_t*)&buff, sizeof(buff));
	if (status != STORAGE_OK) {
#ifdef DEBUG
		LOG_DEBUG(STNG_MODULE_TAG, " load settings: error read\n");
#endif
		return SETTINGS_ERROR;
	}

	if (!_settings_check(&buff)) {
#ifdef DEBUG
		LOG_DEBUG(STNG_MODULE_TAG, " load settings: error check\n");
#endif
		return SETTINGS_ERROR;
	}

	memcpy((uint8_t*)&module_settings, (uint8_t*)&buff, sizeof(module_settings));
#ifdef DEBUG
		LOG_DEBUG(STNG_MODULE_TAG, " load settings: OK\n");
#endif
	return SETTINGS_OK;
}


settings_status_t settings_save() {
	if (!_settings_check(&module_settings)) {
#ifdef DEBUG
		LOG_DEBUG(STNG_MODULE_TAG, " save settings: error check\n");
#endif
		return SETTINGS_ERROR;
	}

	uint32_t stor_addr = 0;
	storage_status_t status = storage_get_first_available_addr(&stor_addr);
	if (status != STORAGE_OK) {
#ifdef DEBUG
		LOG_DEBUG(STNG_MODULE_TAG, " save settings: error storage access\n");
#endif
		return SETTINGS_ERROR;
	}

	util_debug_hex_dump(STNG_MODULE_TAG, (uint8_t*)&module_settings, sizeof(module_settings));

	status = storage_save(stor_addr, (uint8_t*)&module_settings, sizeof(module_settings));
	if (status != STORAGE_OK) {
#ifdef DEBUG
		LOG_DEBUG(STNG_MODULE_TAG, " save settings: error save\n");
#endif
		return SETTINGS_ERROR;
	}


#ifdef DEBUG
	LOG_DEBUG(STNG_MODULE_TAG, " save settings: OK\n");
#endif
	return SETTINGS_OK;
}

void show_settings()
{
	LOG_DEBUG(
		STNG_MODULE_TAG,
		"###################SETTINGS###################\n"
		"Time:             %u-%02u-%02uT%02u:%02u:%02u\n"
		"Device ID:        %lu\n"
		"Server URL:       %s:%s\n"
		"ADC level MIN:    %u\n"
		"ADC level MAX:    %u\n"
		"Liquid level MIN: %lu l\n"
		"Liquid level MAX: %lu l\n"
		"Target:           %lu ml/dn"
		"Pump speed:       %lu ml/h\n"
		"Sleep time:       %lu sec\n"
		"Server log ID:    %lu\n"
		"Pump work:        %lu sec\n"
		"Pump work day:    %lu sec\n"
		"Config ver:       %lu\n"
		"Pump              %s\n"
		"###################SETTINGS###################\n",
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
		module_settings.sleep_time / MILLIS_IN_SECOND,
		module_settings.server_log_id,
		module_settings.pump_work_sec,
		module_settings.pump_work_day_sec,
		module_settings.cf_id,
		module_settings.pump_enabled ? "ON" : "OFF"
	);
}

bool _settings_check(module_settings_t* sttngs)
{
	if (!sttngs->pump_speed) {
		return false;
	}
	if (!sttngs->sleep_time) {
		return false;
	}
	return true;
}
