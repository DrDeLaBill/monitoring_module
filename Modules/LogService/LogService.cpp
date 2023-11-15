/* Copyright © 2023 Georgy E. All rights reserved. */

#include "LogService.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "RecordDB.h"
#include "SettingsDB.h"

#include "pump.h"
#include "defines.h"
#include "sim_module.h"
#include "clock_service.h"
#include "liquid_sensor.h"
#include "pressure_sensor.h"


extern SettingsDB settings;


util_timer_t LogService::logTimer = {};
util_timer_t LogService::settingsTimer = {};
uint32_t LogService::logId = 0;

const char* LogService::TAG               = "LOG";

const char* LogService::T_DASH_FIELD      = "-";
const char* LogService::T_TIME_FIELD      = "t";
const char* LogService::T_COLON_FIELD     = ":";

const char* LogService::TIME_FIELD        = "t";
const char* LogService::CF_ID_FIELD       = "cf_id";
const char* LogService::CF_DATA_FIELD     = "cf";
const char* LogService::CF_DEV_ID_FIELD   = "id";
const char* LogService::CF_PWR_FIELD      = "pwr";
const char* LogService::CF_LTRMIN_FIELD   = "ltrmin";
const char* LogService::CF_LTRMAX_FIELD   = "ltrmax";
const char* LogService::CF_TRGT_FIELD     = "trgt";
const char* LogService::CF_SLEEP_FIELD    = "sleep";
const char* LogService::CF_SPEED_FIELD    = "speed";
const char* LogService::CF_LOGID_FIELD    = "d_hwm";
const char* LogService::CF_CLEAR_FIELD    = "clr";


void LogService::update()
{
	if (if_network_ready()) {
		LogService::sendRequest();
	}

	if (has_http_response()) {
		LogService::parse();
	}

	if (LogService::logTimer.delay != settings.settings.sleep_time) {
		LogService::logTimer.delay = settings.settings.sleep_time;
	}

	if (util_is_timer_wait(&LogService::logTimer)) {
		return;
	}

	if (!settings.settings.sleep_time) {
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "no setting - sleep_time\n");
#endif
		settings.settings.sleep_time = DEFAULT_SLEEPING_TIME;
		return;
	}

	if (LogService::saveNewLog()) {
		settings.settings.pump_work_sec = 0;
		settings.settings.pump_downtime_sec = 0;
	}

	util_timer_start(&logTimer, settings.settings.sleep_time);
}

void LogService::updateSleep(uint32_t time)
{
	settings.settings.sleep_time = time;
	LogService::logTimer.delay = time;
}

void LogService::sendRequest()
{
	char data[LOG_SIZE] = {};
	snprintf(
		data,
		sizeof(data),
		"id=%lu\n"
		"fw_id=%lu\n"
		"cf_id=%lu\n"
		"t=20%02d-%02d-%02dT%02d:%02d:%02d\n",
		settings.settings.id,
		FW_VERSION,
		settings.settings.cf_id,
		get_year(),
		get_month(),
		get_date(),
		get_hour(),
		get_minute(),
		get_second()
	);

	RecordDB record(settings.settings.server_log_id);
	RecordDB::RecordStatus recordStatus = record.loadNext();
	if (recordStatus == RecordDB::RECORD_OK) {
		snprintf(
			data + strlen(data),
			sizeof(data) - strlen(data),
			"d="
				"id=%lu;"
				"t=20%02d-%02d-%02dT%02d:%02d:%02d;"
				"level=%ld;"
				"press_1=%u.%02u;"
//				"press_2=%lu.%02lu;"
				"pumpw=%lu;"
				"pumpd=%lu\r\n",
			record.record.id,
			record.record.time[0], record.record.time[1], record.record.time[2], record.record.time[3], record.record.time[4], record.record.time[5],
			record.record.level / 1000,
			record.record.press_1 / 100, record.record.press_1 % 100,
//			record.record.press_2 / 100, record.record.press_2 % 100,
			record.record.pump_wok_time,
			record.record.pump_downtime
		);
	}

	if (util_is_timer_wait(&LogService::settingsTimer)) {
		return;
	}

	send_http_post(data);
	util_timer_start(&settingsTimer, settingsDelayMs);
	LogService::logId = record.record.id;
}

void LogService::parse()
{
	uint32_t old_log_id = settings.settings.server_log_id;

	char* var_ptr = get_response();
	char* data_ptr = var_ptr;
	if (!var_ptr) {
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "unable to parse response (no response) - [%s]\n", var_ptr);
#endif
		settings.settings.server_log_id = old_log_id;
		return;
	}

	if (!LogService::findParam(&data_ptr, var_ptr, TIME_FIELD)) {
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "unable to parse response (no time) - [%s]\n", var_ptr);
#endif
		settings.settings.server_log_id = old_log_id;
		return;
	}

	if (LogService::updateTime(data_ptr)) {
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "time updated\n");
#endif
	} else {
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "unable to parse response (unable to update time) - [%s]\n", var_ptr);
#endif
		settings.settings.server_log_id = old_log_id;
		return;
	}

	if (settings.settings.server_log_id < LogService::logId) {
		settings.settings.server_log_id = LogService::logId;
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "server log id updated\n");
#endif
	}

	// Parse configuration:
	if (!LogService::findParam(&data_ptr, var_ptr, CF_LOGID_FIELD)) {
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "unable to parse response (log_id not found) - %s\n", var_ptr);
#endif
		settings.settings.server_log_id = old_log_id;
		return;
	}
	settings.settings.server_log_id = atoi(data_ptr);


	PRINT_MESSAGE(LogService::TAG, "Recieved response from the server\n");

	if (!LogService::findParam(&data_ptr, var_ptr, CF_ID_FIELD)) {
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "unable to parse response (cf_id not found) - %s\n", var_ptr);
#endif
		settings.settings.server_log_id = old_log_id;
		return;
	}
	uint32_t new_cf_id = atoi(data_ptr);
	if (new_cf_id == settings.settings.cf_id) {
		LogService::saveResponse();
		return;
	}
	settings.settings.cf_id = new_cf_id;

	if (!LogService::findParam(&data_ptr, var_ptr, CF_DATA_FIELD)) {
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "unable to parse response (no data) - [%s]\n", var_ptr);
#endif
		settings.settings.server_log_id = old_log_id;
		return;
	}
	data_ptr += strlen(CF_DATA_FIELD);
	var_ptr = data_ptr;

	if (LogService::findParam(&data_ptr, var_ptr, CF_PWR_FIELD)) {
		pump_update_enable_state(atoi(data_ptr));
	}

	if (LogService::findParam(&data_ptr, var_ptr, CF_LTRMIN_FIELD)) {
		pump_update_ltrmin(atoi(data_ptr));
	}

	if (LogService::findParam(&data_ptr, var_ptr, CF_LTRMAX_FIELD)) {
		pump_update_ltrmax(atoi(data_ptr));
	}

	if (LogService::findParam(&data_ptr, var_ptr, CF_TRGT_FIELD)) {
		pump_update_target(atoi(data_ptr));
	}

	if (LogService::findParam(&data_ptr, var_ptr, CF_SLEEP_FIELD)) {
		LogService::updateSleep(atoi(data_ptr) * MILLIS_IN_SECOND);
	}

	if (LogService::findParam(&data_ptr, var_ptr, CF_SPEED_FIELD)) {
		pump_update_speed(atoi(data_ptr));
	}

	if (LogService::findParam(&data_ptr, var_ptr, CF_DEV_ID_FIELD)) {
		settings.settings.id = atoi(data_ptr);
	}

	if (LogService::findParam(&data_ptr, var_ptr, CF_CLEAR_FIELD)) {
		if (atoi(data_ptr) == 1) LogService::clearLog();
	}

	LogService::saveResponse();
}

bool LogService::saveNewLog()
{
	RecordDB record(0);
//	cur_record.record.fw_id   = FW_VERSION;
	record.record.cf_id   = settings.settings.cf_id;
	record.record.level   = get_liquid_liters() * 1000;
	record.record.press_1 = get_press();
//	cur_record.record.press_2 = get_second_press();

	record.record.time[0] = get_year() % 100;
	record.record.time[1] = get_month();
	record.record.time[2] = get_date();
	record.record.time[3] = get_hour();
	record.record.time[4] = get_minute();
	record.record.time[5] = get_second();

	record.record.pump_wok_time = settings.settings.pump_work_sec;
	record.record.pump_downtime = settings.settings.pump_downtime_sec;

	return record.save() == RecordDB::RECORD_OK;
}

bool LogService::findParam(char** dst, const char* src, const char* param)
{
	char search_param[CHAR_PARAM_SIZE] = {0};
	if (strlen(param) > sizeof(search_param) - 3) {
		return false;
	}

	char* ptr = NULL;

	snprintf(search_param, sizeof(search_param), "\n%s=", param);
	ptr = strnstr(src, search_param, strlen(src));
	if (ptr) {
		goto do_success;
	}

	snprintf(search_param, sizeof(search_param), ";%s=", param);
	ptr = strnstr(src, search_param, strlen(src));
	if (ptr) {
		goto do_success;
	}

	snprintf(search_param, sizeof(search_param), "=%s=", param);
	ptr = strnstr(src, search_param, strlen(src));
	if (ptr) {
		goto do_success;
	}

	goto do_error;

do_success:
	*dst = ptr + strlen(search_param);
	return true;

do_error:
	return false;
}

bool LogService::updateTime(char* data)
{
	// Parse time
	DateTime datetime = {0};

	char* data_ptr = data;
	if (!data_ptr) {
		return false;
	}
	datetime.year = atoi(data_ptr) % 100;

	data_ptr = strnstr(data_ptr, T_DASH_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_DASH_FIELD);
	datetime.month = (uint8_t)atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_DASH_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_DASH_FIELD);
	datetime.date = atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_TIME_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_TIME_FIELD);
	datetime.hour = atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_COLON_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_COLON_FIELD);
	datetime.minute = atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_COLON_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_COLON_FIELD);
	datetime.second = atoi(data_ptr);

	if(!save_datetime(&datetime)) {
#if LOG_SERVICE_BEDUG
		LOG_TAG_BEDUG(LogService::TAG, "parse error - unable to update datetime\n");
#endif
		return false;
	}

	return true;
}

void LogService::clearLog()
{
	settings.settings.server_log_id = 0;
	settings.settings.cf_id = 0;
	settings.settings.pump_work_sec = 0;
	settings.settings.pump_work_day_sec = 0;
	settings.settings.pump_downtime_sec = 0;
	// TODO: clear log in EEPROM
	settings.save();
}

void LogService::saveResponse()
{
#if LOG_SERVICE_BEDUG
	LOG_TAG_BEDUG(LogService::TAG, "configuration updated\n");
#endif

	if (settings.save() == SettingsDB::SETTINGS_OK) {
		PRINT_MESSAGE(LogService::TAG, "New settings have been received from the server\n");
	} else {
		PRINT_MESSAGE(LogService::TAG, "Unable to recieve new settings from the server\n");
		settings.settings.cf_id = 0;
	}
}
