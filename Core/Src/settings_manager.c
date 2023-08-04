#include "settings_manager.h"


#include <string.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

#include "utils.h"
#include "defines.h"
#include "storage_data_manager.h"
#include "command_manager.h"
#include "clock_service.h"


const char* STNG_MODULE_TAG = "STNG";

const char default_server_url[CHAR_SETIINGS_SIZE] = "urv.a.izhpt.com";
const char default_server_port[CHAR_SETIINGS_SIZE] = "80";


module_settings_t module_settings;


bool _settings_check(module_settings_t* sttngs);


settings_status_t settings_reset() {
	LOG_DEBUG(STNG_MODULE_TAG, "SET DEFAULT SETTINGS\n");
	module_settings.id = 1;
	memset(module_settings.server_url, 0, sizeof(module_settings.server_url));
	memset(module_settings.server_port, 0, sizeof(module_settings.server_port));
	strncpy(module_settings.server_url, default_server_url, sizeof(module_settings.server_url));
	strncpy(module_settings.server_port, default_server_port, sizeof(module_settings.server_port));
	module_settings.tank_liters_min = MIN_TANK_LTR;
	module_settings.tank_liters_max = MAX_TANK_LTR;
	module_settings.tank_ADC_max = MAX_TANK_VOLUME;
	module_settings.tank_ADC_min = MIN_TANK_VOLUME;
	module_settings.milliliters_per_day = MIN_TANK_LTR;
	module_settings.pump_speed = 0;
	module_settings.sleep_time = DEFAULT_SLEEPING_TIME;
	module_settings.pump_enabled = true;
	module_settings.server_log_id = 0;
	module_settings.cf_id = CF_VERSION_DEFAULT;
	module_settings.pump_work_day_sec = 0;
	module_settings.pump_downtime_sec = 0;
	module_settings.pump_work_sec = 0;
	module_settings.pump_log_date = get_date();
	module_settings.is_first_start = 1;
	return settings_save();
}


settings_status_t settings_load() {
	module_settings_t buff;
	memset(&buff, 0, sizeof(buff));

	LOG_DEBUG(STNG_MODULE_TAG, "load settings: begin\n");
	uint32_t stor_addr = 0;
	storage_status_t status = storage_get_first_available_addr(&stor_addr);
	if (status != STORAGE_OK) {
		LOG_DEBUG(STNG_MODULE_TAG, "load settings: error storage access\n");
		return SETTINGS_ERROR;
	}

	status = storage_load(stor_addr, (uint8_t*)&buff, sizeof(buff));
	if (status != STORAGE_OK) {
		LOG_DEBUG(STNG_MODULE_TAG, "load settings: error read\n");
		return SETTINGS_ERROR;
	}

	if (!_settings_check(&buff)) {
		LOG_DEBUG(STNG_MODULE_TAG, "load settings: error check\n");
		return SETTINGS_ERROR;
	}

	memcpy((uint8_t*)&module_settings, (uint8_t*)&buff, sizeof(module_settings));
	LOG_DEBUG(STNG_MODULE_TAG, "load settings: end - OK\n");
	return SETTINGS_OK;
}


settings_status_t settings_save() {
	LOG_DEBUG(STNG_MODULE_TAG, "save settings: begin\n");

	if (!_settings_check(&module_settings)) {
		LOG_DEBUG(STNG_MODULE_TAG, "save settings: error check\n");
		return SETTINGS_ERROR;
	}

	uint32_t stor_addr = 0;
	storage_status_t status = storage_get_first_available_addr(&stor_addr);
	if (status != STORAGE_OK) {
		LOG_DEBUG(STNG_MODULE_TAG, "save settings: error storage access\n");
		return SETTINGS_ERROR;
	}

	status = storage_save(stor_addr, (uint8_t*)&module_settings, sizeof(module_settings));
	if (status != STORAGE_OK) {
		LOG_DEBUG(STNG_MODULE_TAG, "save settings: error save %02x\n", status);
		return SETTINGS_ERROR;
	}

	LOG_DEBUG(STNG_MODULE_TAG, "save settings: end - OK\n");
	return SETTINGS_OK;
}

void show_settings()
{
	LOG_DEBUG(STNG_MODULE_TAG, LOG_DEBUG_SETTINGS_FORMAT);
}

bool _settings_check(module_settings_t* sttngs)
{
	if (!sttngs->sleep_time) {
		return false;
	}
	return true;
}
