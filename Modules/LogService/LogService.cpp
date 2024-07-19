/* Copyright © 2023 Georgy E. All rights reserved. */

#include "LogService.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "pump.h"
#include "clock.h"
#include "defines.h"
#include "settings.h"
#include "sim_module.h"
#include "liquid_sensor.h"
#include "pressure_sensor.h"

#include "RecordDB.h"


extern settings_t settings;


util_old_timer_t LogService::logTimer = {};
util_old_timer_t LogService::settingsTimer = {};
uint32_t LogService::logId = 0;
std::unique_ptr<RecordDB> LogService::nextRecord = std::make_unique<RecordDB>(0);
bool LogService::newRecordLoaded = false;


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

	if (LogService::logTimer.delay != settings.sleep_time) {
		LogService::logTimer.delay = settings.sleep_time;
	}

	if (util_old_timer_wait(&LogService::logTimer)) {
		return;
	}

	if (!settings.sleep_time) {
#if LOG_SERVICE_BEDUG
		printTagLog(LogService::TAG, "no setting - sleep_time\n");
#endif
		settings.sleep_time = DEFAULT_SLEEPING_TIME;
		return;
	}

	LogService::saveNewLog();

	util_old_timer_start(&logTimer, settings.sleep_time);
}

void LogService::updateSleep(uint32_t time)
{
	settings.sleep_time = time;
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
		settings.id,
		FW_VERSION,
		settings.cf_id,
		clock_get_year(),
		clock_get_month(),
		clock_get_date(),
		clock_get_hour(),
		clock_get_minute(),
		clock_get_second()
	);

	RecordDB::RecordStatus recordStatus = RecordDB::RECORD_ERROR;
	if (!newRecordLoaded && is_new_data_saved()) {
		nextRecord   = std::make_unique<RecordDB>(static_cast<uint32_t>(settings.server_log_id));
		recordStatus = nextRecord->loadNext();
	}
	if (recordStatus == RecordDB::RECORD_NO_LOG) {
		set_new_data_saved(false);
	}
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
			nextRecord->record.id,
			nextRecord->record.time[0], nextRecord->record.time[1], nextRecord->record.time[2], nextRecord->record.time[3], nextRecord->record.time[4], nextRecord->record.time[5],
			nextRecord->record.level / 1000,
			nextRecord->record.press_1 / 100, nextRecord->record.press_1 % 100,
//			nextRecord->record.press_2 / 100, record.record.press_2 % 100,
			nextRecord->record.pump_wok_time,
			nextRecord->record.pump_downtime
		);
		newRecordLoaded = true;
	}

	if (recordStatus != RecordDB::RECORD_OK && util_old_timer_wait(&LogService::settingsTimer)) {
		return;
	}

	if (recordStatus == RecordDB::RECORD_OK) {
		newRecordLoaded = false;
	}


#if LOG_SERVICE_BEDUG
	printTagLog(TAG, "request: %s\n", data);
#endif

	send_http_post(data);
	util_old_timer_start(&settingsTimer, settingsDelayMs);
	LogService::logId = nextRecord->record.id;
}

void LogService::parse()
{
	char* var_ptr = get_response();
	char* data_ptr = var_ptr;

#if LOG_SERVICE_BEDUG
	printTagLog(TAG, "response: %s\n", var_ptr);
#endif

	if (!var_ptr) {
#if LOG_SERVICE_BEDUG
		printTagLog(LogService::TAG, "unable to parse response (no response) - [%s]\n", var_ptr);
#endif
		return;
	}

	if (!LogService::findParam(&data_ptr, var_ptr, TIME_FIELD)) {
#if LOG_SERVICE_BEDUG
		printTagLog(LogService::TAG, "unable to parse response (no time) - [%s]\n", var_ptr);
#endif
		return;
	}

	if (LogService::updateTime(data_ptr)) {
#if LOG_SERVICE_BEDUG
		printTagLog(LogService::TAG, "time updated\n");
#endif
	} else {
#if LOG_SERVICE_BEDUG
		printTagLog(LogService::TAG, "unable to parse response (unable to update time) - [%s]\n", var_ptr);
#endif
		return;
	}

	if (settings.server_log_id < LogService::logId) {
		settings.server_log_id = LogService::logId;
#if LOG_SERVICE_BEDUG
		printTagLog(LogService::TAG, "server log id updated\n");
#endif
	}

	// Parse configuration:
	if (!LogService::findParam(&data_ptr, var_ptr, CF_LOGID_FIELD)) {
#if LOG_SERVICE_BEDUG
		printTagLog(LogService::TAG, "unable to parse response (log_id not found) - %s\n", var_ptr);
#endif
		return;
	}
	settings.server_log_id = atoi(data_ptr);


	printTagLog(LogService::TAG, "Recieved response from the server\n");

	if (!LogService::findParam(&data_ptr, var_ptr, CF_ID_FIELD)) {
#if LOG_SERVICE_BEDUG
		printTagLog(LogService::TAG, "unable to parse response (cf_id not found) - %s\n", var_ptr);
#endif
		LogService::saveResponse();
		return;
	}
	uint32_t new_cf_id = atoi(data_ptr);
	if (new_cf_id == settings.cf_id) {
		LogService::saveResponse();
		return;
	}
	settings.cf_id = new_cf_id;

	if (!LogService::findParam(&data_ptr, var_ptr, CF_DATA_FIELD)) {
#if LOG_SERVICE_BEDUG
		printTagLog(LogService::TAG, "unable to parse response (no data) - [%s]\n", var_ptr);
#endif
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
		settings.id = atoi(data_ptr);
	}

	if (LogService::findParam(&data_ptr, var_ptr, CF_CLEAR_FIELD)) {
		if (atoi(data_ptr) == 1) LogService::clearLog();
	}

	LogService::saveResponse();
}

void LogService::saveNewLog()
{
	RecordDB record(0);
//	cur_record.record.fw_id   = FW_VERSION;
	record.record.cf_id   = settings.cf_id;
	record.record.level   = get_liquid_level() * 1000;
	record.record.press_1 = get_press();
//	cur_record.record.press_2 = get_second_press();

	record.record.time[0] = clock_get_year() % 100;
	record.record.time[1] = clock_get_month();
	record.record.time[2] = clock_get_date();
	record.record.time[3] = clock_get_hour();
	record.record.time[4] = clock_get_minute();
	record.record.time[5] = clock_get_second();

	record.record.pump_wok_time = settings.pump_work_sec;
	record.record.pump_downtime = settings.pump_downtime_sec;

	if (record.save() == RecordDB::RECORD_OK) {
		settings.pump_work_sec = 0;
		settings.pump_downtime_sec = 0;
		set_settings_update_status(true);
	}
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
	RTC_DateTypeDef date = {};
	RTC_TimeTypeDef time = {};

	char* data_ptr = data;
	if (!data_ptr) {
		return false;
	}
	date.Year = (uint8_t)(atoi(data_ptr) % 100);

	data_ptr = strnstr(data_ptr, T_DASH_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_DASH_FIELD);
	date.Month = (uint8_t)atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_DASH_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_DASH_FIELD);
	date.Date = (uint8_t)atoi(data_ptr);

	clock_save_date(&date);

	data_ptr = strnstr(data_ptr, T_TIME_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_TIME_FIELD);
	time.Hours = (uint8_t)atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_COLON_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_COLON_FIELD);
	time.Minutes = (uint8_t)atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_COLON_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		return false;
	}
	data_ptr += strlen(T_COLON_FIELD);
	time.Seconds = (uint8_t)atoi(data_ptr);

	clock_save_time(&time);

	return true;
}

void LogService::clearLog()
{
	settings.server_log_id = 0;
	settings.cf_id = 0;
	settings.pump_work_sec = 0;
	settings.pump_work_day_sec = 0;
	settings.pump_downtime_sec = 0;
	// TODO: clear log in EEPROM
	set_settings_update_status(true);
}

void LogService::saveResponse()
{
#if LOG_SERVICE_BEDUG
	printTagLog(LogService::TAG, "configuration updated\n");
#endif
	set_settings_update_status(true);
}
