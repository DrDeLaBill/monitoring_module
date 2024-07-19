/* Copyright © 2023 Georgy E. All rights reserved. */

#include "settings.h"

#include <string.h>
#include <stdbool.h>

#include "log.h"
#include "main.h"
#include "utils.h"
#include "clock.h"
#include "defines.h"
#include "hal_defs.h"


static const char SETTINGS_TAG[] = "STNG";

settings_t settings = { 0 };

settings_info_t stngs_info = {
	.settings_initialized = false,
	.settings_saved       = false,
	.settings_updated     = false,
	.saved_new_data       = true
};


settings_t* settings_get()
{
	return &settings;
}

void settings_set(settings_t* other)
{
	memcpy((uint8_t*)&settings, (uint8_t*)other, sizeof(settings));
}

void settings_reset(settings_t* other)
{
	printTagLog(SETTINGS_TAG, "Reset settings");

	other->id = 1;
	memset(other->server_url, 0, sizeof(other->server_url));
	memset(other->server_port, 0, sizeof(other->server_port));
	strncpy(other->server_url, defaultUrl, sizeof(other->server_url));
	strncpy(other->server_port, defaultPort, sizeof(other->server_port));
	other->tank_liters_min = MIN_TANK_LTR;
	other->tank_liters_max = MAX_TANK_LTR;
	other->tank_ADC_max = MAX_TANK_VOLUME;
	other->tank_ADC_min = MIN_TANK_VOLUME;
	other->pump_target = 0;
	other->pump_speed = 0;
	other->sleep_time = DEFAULT_SLEEPING_TIME;
	other->pump_enabled = true;
	other->server_log_id = 0;
	other->cf_id = CF_VERSION_DEFAULT;
	other->pump_work_day_sec = 0;
	other->pump_downtime_sec = 0;
	other->pump_work_sec = 0;
	other->pump_log_date = clock_get_date();
	other->is_first_start = 1;
}

uint32_t settings_size()
{
	return sizeof(settings_t);
}

bool settings_check(settings_t* other)
{
	return other->sleep_time > 0;
}

void settings_show()
{
	gprint(
		"\n####################SETTINGS####################\n" \
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
		"####################SETTINGS####################\n", \
		clock_get_year() % 100, \
		clock_get_month(), \
		clock_get_date(), \
		clock_get_hour(), \
		clock_get_minute(), \
		clock_get_second(), \
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
}

void settings_set_cf_id(uint32_t cf_id)
{
    if (cf_id) {
        settings.cf_id = cf_id;
    }
}

void settings_set_id(uint32_t id)
{
	if (id) {
		settings.id = id;
	}
}
bool is_settings_saved()
{
	return stngs_info.settings_saved;
}

bool is_settings_updated()
{
	return stngs_info.settings_updated;
}

bool is_settings_initialized()
{
	return stngs_info.settings_initialized;
}

bool is_new_data_saved()
{
	return stngs_info.saved_new_data;
}

void set_settings_initialized()
{
	stngs_info.settings_initialized = true;
}

void set_settings_save_status(bool state)
{
	if (state) {
		stngs_info.settings_updated = false;
	}
    set_new_data_saved(state);
	stngs_info.settings_saved = state;
}

void set_settings_update_status(bool state)
{
	if (state) {
		stngs_info.settings_saved = false;
	}
	stngs_info.settings_updated = state;
}

void set_new_data_saved(bool state)
{
	stngs_info.saved_new_data = state;
}
