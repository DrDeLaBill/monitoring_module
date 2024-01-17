/* Copyright © 2023 Georgy E. All rights reserved. */

#include "SettingsDB.h"

#include <algorithm>
#include <string.h>

#include "StorageAT.h"

#include "utils.h"
#include "clock_service.h"


#define EXIT_CODE(_code_) { return _code_; }


extern StorageAT storage;


const char* SettingsDB::SETTINGS_PREFIX = "STG";
const char* SettingsDB::TAG = "STG";

const char SettingsDB::defaultUrl[CHAR_SETIINGS_SIZE] = "urv.a.izhpt.com";
const char SettingsDB::defaultPort[CHAR_SETIINGS_SIZE] = "80";


SettingsDB::SettingsDB()
{
    memset(reinterpret_cast<void*>(&(this->settings)), 0, sizeof(this->settings));
    memset(reinterpret_cast<void*>(&(this->info)), 0, sizeof(this->info));
    this->info.settings_loaded = false;
    this->info.saved_new_log = true;
}

SettingsDB::SettingsStatus SettingsDB::load()
{
    uint32_t address = 0;
    StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, SETTINGS_PREFIX, 1);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error load settings");
#endif
        this->info.settings_loaded = false;
        EXIT_CODE(SETTINGS_ERROR);
    }

    Settings tmpSettings = {};
    status = storage.load(address, reinterpret_cast<uint8_t*>(&tmpSettings), sizeof(tmpSettings));
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error load settings");
#endif
        this->info.settings_loaded = false;
        EXIT_CODE(SETTINGS_ERROR);
    }

    if (!this->check(&tmpSettings)) {
    	this->show();
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error settings check");
#endif
        this->info.settings_loaded = false;
        EXIT_CODE(SETTINGS_ERROR);
    }

    memcpy(reinterpret_cast<void*>(&(this->settings)), reinterpret_cast<void*>(&tmpSettings), sizeof(this->settings));

    this->info.settings_loaded = true;

#if SETTINGS_BEDUG
    LOG_TAG_BEDUG(SettingsDB::TAG, "settings loaded");
    this->show();
#endif

    EXIT_CODE(SETTINGS_OK);
}

SettingsDB::SettingsStatus SettingsDB::save()
{
    uint32_t address = 0;
    StorageFindMode mode = FIND_MODE_EQUAL;
    StorageStatus status = storage.find(mode, &address, SETTINGS_PREFIX, 1);
    if (status == STORAGE_NOT_FOUND) {
    	mode = FIND_MODE_EMPTY;
        status = storage.find(mode, &address, SETTINGS_PREFIX, 1);
    }
    while (status == STORAGE_NOT_FOUND) {
        // Search for any address
        mode = FIND_MODE_NEXT;
    	status = storage.find(mode, &address, "", 1);
    	if (status != STORAGE_OK) {
    		continue;
    	}
    }
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error find settings (address=%lu)", address);
#endif
        EXIT_CODE(SETTINGS_ERROR);
    }

    if (mode != FIND_MODE_EMPTY) {
    	status = storage.deleteData(address);
    }
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error delete settings (address=%lu)", address);
#endif
        EXIT_CODE(SETTINGS_ERROR);
    }

    status = storage.save(address, SETTINGS_PREFIX, 1, reinterpret_cast<uint8_t*>(&(this->settings)), sizeof(this->settings));
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error save settings (address=%lu)", address);
#endif
        EXIT_CODE(SETTINGS_ERROR);
    }

    info.settings_loaded = false;

#if SETTINGS_BEDUG
    LOG_TAG_BEDUG(SettingsDB::TAG, "settings saved (address=%lu)", address);
#endif

    EXIT_CODE(this->load());
}

SettingsDB::SettingsStatus SettingsDB::reset()
{
#if SETTINGS_BEDUG
    LOG_TAG_BEDUG(SettingsDB::TAG, "reset settings");
#endif

	settings.id = 1;
	memset(settings.server_url, 0, sizeof(settings.server_url));
	memset(settings.server_port, 0, sizeof(settings.server_port));
	strncpy(settings.server_url, defaultUrl, sizeof(settings.server_url));
	strncpy(settings.server_port, defaultPort, sizeof(settings.server_port));
	settings.tank_liters_min = MIN_TANK_LTR;
	settings.tank_liters_max = MAX_TANK_LTR;
	settings.tank_ADC_max = MAX_TANK_VOLUME;
	settings.tank_ADC_min = MIN_TANK_VOLUME;
	settings.pump_target = 0;
	settings.pump_speed = 0;
	settings.sleep_time = DEFAULT_SLEEPING_TIME;
	settings.pump_enabled = true;
	settings.server_log_id = 0;
	settings.cf_id = CF_VERSION_DEFAULT;
	settings.pump_work_day_sec = 0;
	settings.pump_downtime_sec = 0;
	settings.pump_work_sec = 0;
	settings.pump_log_date = get_date();
	settings.is_first_start = 1;

    return this->save();
}

bool SettingsDB::isLoaded()
{
    return this->info.settings_loaded;
}

bool SettingsDB::check(Settings* settings)
{
	return settings->sleep_time;
}

void SettingsDB::show()
{
#if SETTINGS_BEDUG
	LOG_BEDUG(
		"\n\n####################SETTINGS####################\n" \
		"Time:             20%02u-%02u-%02uT%02u:%02u:%02u\n" \
		"Device ID:        %lu\n" \
		"Server URL:       %s:%s\n" \
		"ADC level MIN:    %lu\n" \
		"ADC level MAX:    %lu\n" \
		"Liquid level MIN: %lu l\n" \
		"Liquid level MAX: %lu l\n" \
		"Target:           %lu l/d\n" \
		"Sleep time:       %lu sec\n" \
		"Server log ID:    %lu\n" \
		"Pump speed:       %lu ml/h\n" \
		"Pump work:        %lu sec\n" \
		"Pump work day:    %lu sec\n" \
		"Config ver:       %lu\n" \
		"Pump              %s\n" \
		"####################SETTINGS####################\n\n", \
		get_year() % 100, \
		get_month(), \
		get_date(), \
		get_hour(), \
		get_minute(), \
		get_second(), \
		settings.id, \
		settings.server_url, \
		settings.server_port, \
		settings.tank_ADC_min, \
		settings.tank_ADC_max, \
		settings.tank_liters_min / MILLILITERS_IN_LITER, \
		settings.tank_liters_max / MILLILITERS_IN_LITER, \
		settings.pump_target / MILLILITERS_IN_LITER, \
		settings.sleep_time / MILLIS_IN_SECOND, \
		settings.server_log_id, \
		settings.pump_speed, \
		settings.pump_work_sec, \
		settings.pump_work_day_sec, \
		settings.cf_id, \
		settings.pump_enabled ? "ON" : "OFF"
	);
#endif
}

void SettingsDB::set_cf_id(uint32_t cf_id)
{
    if (cf_id) {
        settings.cf_id = cf_id;
    }
}

void SettingsDB::set_id(uint32_t id)
{
    if (id) {
        settings.id = id;
    }
}
