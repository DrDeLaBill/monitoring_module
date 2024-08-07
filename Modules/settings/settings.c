/* Copyright © 2023 Georgy E. All rights reserved. */

#include "settings.h"

#include <string.h>
#include <stdbool.h>

#include "glog.h"
#include "main.h"
#include "gutils.h"
#include "clock.h"
#include "defines.h"
#include "system.h"
#include "hal_defs.h"


static const char SETTINGS_TAG[] = "STNG";

const char defaultUrl[CHAR_SETIINGS_SIZE]  = "urv.iot.turtton.ru";

settings_t settings = { 0 };


settings_t* settings_get()
{
	return &settings;
}

void settings_set(settings_t* other)
{
	memcpy((uint8_t*)&settings, (uint8_t*)other, sizeof(settings));
}

uint32_t settings_size()
{
	return sizeof(settings_t);
}

bool settings_check(settings_t* other)
{
	if (other->bedacode != BEDACODE) {
		return false;
	}
	if (other->dv_type != DEVICE_TYPE) {
		return false;
	}
	if (other->fw_id != FW_VERSION) {
		return false;
	}
	if (other->sw_id != SW_VERSION) {
		return false;
	}

	return other->sleep_time > 0;
}

void settings_repair(settings_t* other)
{
	if (other->bedacode != BEDACODE) {
		settings_v2_t old_stng = {0};
		memcpy((void*)&old_stng, (void*)other, __min(sizeof(settings_v2_t), sizeof(settings_t)));
		memset((void*)other, 0, sizeof(settings_t));

		settings_reset(other);

#if SETTINGS_BEDUG
		printTagLog(SETTINGS_TAG, "Repair settings");
#endif
		other->bedacode = BEDACODE;
		other->dv_type  = 1;
		other->fw_id    = 2;
		other->sw_id    = 3;

		other->cf_id = old_stng.cf_id;
		memset(other->url, 0, sizeof(other->url));
		strncpy(other->url, old_stng.server_url, sizeof(other->url));
		other->tank_ltr_min = old_stng.tank_liters_min;
		other->tank_ltr_max = old_stng.tank_liters_max;
		other->tank_ADC_max = old_stng.tank_ADC_max;
		other->tank_ADC_min = old_stng.tank_ADC_min;
		other->pump_target_ml = old_stng.pump_target;
		other->pump_speed = old_stng.pump_speed;
		other->sleep_time = old_stng.sleep_time;
		other->pump_enabled = old_stng.pump_enabled;
		other->server_log_id = old_stng.server_log_id;
		other->pump_work_day_sec = old_stng.pump_work_day_sec;
		other->pump_downtime_sec = old_stng.pump_downtime_sec;
		other->pump_work_sec = old_stng.pump_work_sec;
		other->pump_log_date = old_stng.pump_log_date;
		other->registrated = 0;
		other->calibrated = 0;
	}

	if (other->sw_id == 3) {
		other->sw_id         = 4;

		other->tank_ltr_min /= MILLILITERS_IN_LITER;
		other->tank_ltr_max /= MILLILITERS_IN_LITER;
	}

	if (!settings_check(other)) {
		settings_reset(other);
	}
}

void settings_reset(settings_t* other)
{
#if SETTINGS_BEDUG
	printTagLog(SETTINGS_TAG, "Reset settings");
#endif

	other->bedacode = BEDACODE;
	other->dv_type = DEVICE_TYPE;
	other->fw_id = FW_VERSION;
	other->sw_id = SW_VERSION;

	other->cf_id = CF_VERSION;
	memset(other->url, 0, sizeof(other->url));
	strncpy(other->url, defaultUrl, sizeof(other->url));
	other->tank_ltr_min = MIN_TANK_LTR;
	other->tank_ltr_max = MAX_TANK_LTR;
	other->tank_ADC_max = MAX_TANK_VOLUME;
	other->tank_ADC_min = MIN_TANK_VOLUME;
	other->pump_target_ml = 0;
	other->pump_speed = 0;
	other->sleep_time = DEFAULT_SLEEPING_TIME;
	other->pump_enabled = true;
	other->server_log_id = 0;
	other->pump_work_day_sec = 0;
	other->pump_downtime_sec = 0;
	other->pump_work_sec = 0;
	other->pump_log_date = clock_get_date();
	other->registrated = 0;
	other->calibrated = 0;
}

void settings_show()
{
#if SETTINGS_BEDUG
	gprint(
		"\n####################SETTINGS####################\n"
		"Time:             %s\n"
		"Device ID:        %s\n"
		"Server URL:       %s\n"
		"Sleep time:       %lu sec\n"
		"Target:           %lu l/d\n"
		"Pump speed:       %lu ml/h\n"
		"Pump work:        %lu sec\n"
		"Pump work day:    %lu sec\n"
		"Pump              %s\n"
		"ADC level MIN:    %lu\n"
		"ADC level MAX:    %lu\n"
		"Liquid level MIN: %lu l\n"
		"Liquid level MAX: %lu l\n"
		"Server log ID:    %lu\n"
		"Config ver:       %lu\n"
		"####################SETTINGS####################\n",
		get_clock_time_format(),
		get_system_serial_str(),
		settings.url,
		settings.sleep_time / MILLIS_IN_SECOND,
		settings.pump_target_ml / MILLILITERS_IN_LITER,
		settings.pump_speed,
		settings.pump_work_sec,
		settings.pump_work_day_sec,
		settings.pump_enabled ? "ON" : "OFF",
		settings.tank_ADC_min,
		settings.tank_ADC_max,
		settings.tank_ltr_min,
		settings.tank_ltr_max,
		settings.server_log_id,
		settings.cf_id
	);
#else
	gprint(
		"\n####################SETTINGS####################\n"
		"Time:             %s\n"
		"Device ID:        %s\n"
		"Server URL:       %s\n"
		"Sleep time:       %lu sec\n"
		"Target:           %lu l/d\n"
		"Pump speed:       %lu ml/h\n"
		"Pump work:        %lu sec\n"
		"Pump work day:    %lu sec\n"
		"Pump              %s\n"
		"####################SETTINGS####################\n",
		get_clock_time_format(),
		get_system_serial_str(),
		settings.url,
		settings.sleep_time / MILLIS_IN_SECOND,
		settings.pump_target_ml / MILLILITERS_IN_LITER,
		settings.pump_speed,
		settings.pump_work_sec,
		settings.pump_work_day_sec,
		settings.pump_enabled ? "ON" : "OFF"
	);
#endif
}
