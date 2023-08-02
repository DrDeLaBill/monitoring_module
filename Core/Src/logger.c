/*
 * record.c
 *
 *  Created on: 19 дек. 2022 г.
 *      Author: DrDeLaBill
 */

#include "logger.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "defines.h"
#include "utils.h"
#include "liquid_sensor.h"
#include "record_manager.h"
#include "settings_manager.h"
#include "sim_module.h"
#include "clock_service.h"
#include "pressure_sensor.h"
#include "pump.h"


#define UPDATE_SETTINGS_TIME 60000


void _send_http_log();
void _make_measurements();
void _show_measurements();
void _parse_response();
void _timer_start();
record_status_t _load_current_log();
void _start_error_timer();
void _start_settings_timer();


const char* LOG_TAG         = "LOG";

const char* T_DASH_FIELD    = "-";
const char* T_TIME_FIELD    = "t";
const char* T_COLON_FIELD   = ":";

const char* TIME_FIELD      = "t=";
const char* CF_ID_FIELD     = "cf_id=";
const char* CF_DATA_FIELD   = "cf=";
const char* CF_MOD_ID_FIELD = "id=";
const char* CF_PWR_FIELD    = "pwr=";
const char* CF_LTRMIN_FIELD = "ltrmin=";
const char* CF_LTRMAX_FIELD = "ltrmax=";
const char* CF_TRGT_FIELD   = "trgt=";
const char* CF_SLEEP_FIELD  = "sleep=";
const char* CF_SPEED_FIELD  = "speed=";
const char* CF_LOGID_FIELD  = "d_hwm=";
const char* CF_CLEAR_FIELD  = "clr=";


dio_timer_t log_timer;
dio_timer_t error_read_timer;
dio_timer_t settings_timer;
uint32_t sended_log_id;


void logger_manager_begin()
{
	update_log_timer();
	_start_error_timer();
	_start_settings_timer();
	sended_log_id = 0;
}

void logger_proccess()
{
	if (if_network_ready()) {
		_send_http_log();
	}

	if (has_http_response()) {
		_parse_response();
	}

	if (log_timer.delay != module_settings.sleep_time) {
		log_timer.delay = module_settings.sleep_time;
	}

	if (util_is_timer_wait(&log_timer)) {
		return;
	}

	if (!module_settings.sleep_time) {
		LOG_DEBUG(LOG_TAG, "no setting - sleep_time\n");
		module_settings.sleep_time = DEFAULT_SLEEPING_TIME;
		return;
	}

	_make_measurements();
	_show_measurements();
	record_status_t status = record_save();
	if (status == RECORD_OK) {
		LOG_MESSAGE(LOG_TAG, "saved new log\n");
	} else {
		LOG_MESSAGE(LOG_TAG, "error saving new log\n");
	}
	update_log_timer();
}

void update_log_timer()
{
	util_timer_start(&log_timer, module_settings.sleep_time);
}

void update_log_sleep(uint32_t time)
{
	module_settings.sleep_time = time;
	log_timer.delay = time;
}

void clear_log()
{
	module_settings.server_log_id = 0;
	module_settings.pump_work_sec = 0;
	settings_save();
}

void _send_http_log()
{
	char data[LOG_SIZE] = {};
	snprintf(
		data,
		sizeof(data),
		"id=%lu\n"
		"fw_id=%lu\n"
		"cf_id=%lu\n"
		"t=20%02d-%02d-%02dT%02d:%02d:%02d\n",
		module_settings.id,
		FW_VERSION,
		module_settings.cf_id,
		get_year(),
		get_month(),
		get_date(),
		get_date(),
		get_minute(),
		get_second()
	);

	record_status_t record_res = next_record_load();

	if (record_res == RECORD_ERROR) {
		record_res = _load_current_log();
		log_record.id = module_settings.server_log_id;
	}

	if (record_res == RECORD_OK) {
		snprintf(
			data + strlen(data),
			sizeof(data) - strlen(data),
			"d="
				"id=%lu;"
				"t=20%02d-%02d-%02dT%02d:%02d:%02d;"
				"level=%ld;"
				"press_1=%lu.%02lu;"
//				"press_2=%lu.%02lu;"
				"pumpw=%lu;"
				"pumpd=%lu\r\n",
			log_record.id,
			log_record.time[0], log_record.time[1], log_record.time[2], log_record.time[3], log_record.time[4], log_record.time[5],
			log_record.level / 1000,
			log_record.press_1 / 100, log_record.press_1 % 100,
//			cur_log_record.press_2 / 100, cur_log_record.press_2 % 100,
			module_settings.pump_work_sec,
			module_settings.pump_downtime_sec
		);
	}

	if (record_res == RECORD_ERROR) {
		return;
	}

	if (record_res == RECORD_NO_LOG && util_is_timer_wait(&settings_timer)) {
		return;
	}

	send_http_post(data);
	_start_settings_timer();
	_start_error_timer();
	sended_log_id = log_record.id;
}

record_status_t _load_current_log()
{
	if (util_is_timer_wait(&error_read_timer)) {
		return RECORD_NO_LOG;
	}
#if LOGGER_DEBUG
	LOG_DEBUG(LOG_TAG, "load current log\n");
#endif
	_make_measurements();
	return RECORD_OK;
}

void _start_error_timer()
{
	util_timer_start(&error_read_timer, module_settings.sleep_time);
}

void _start_settings_timer()
{
	util_timer_start(&settings_timer, UPDATE_SETTINGS_TIME);
}

void _make_measurements()
{
//	cur_log_record.fw_id   = FW_VERSION;
	log_record.cf_id   = module_settings.cf_id;
	record_get_new_id(&log_record.id);
	log_record.level   = get_liquid_liters();
	log_record.press_1 = get_first_press();
//	cur_log_record.press_2 = get_second_press();

	log_record.time[0] = get_year() % 100;
	log_record.time[1] = get_month();
	log_record.time[2] = get_date();
	log_record.time[3] = get_hour();
	log_record.time[4] = get_minute();
	log_record.time[5] = get_second();
}

void _show_measurements()
{
	LOG_DEBUG(
		LOG_TAG,
		"\n"
#if LOGGER_DEBUG
		"ID:      %lu\n"
#endif
		"Time:    20%02u-%02u-%02uT%02u:%02u:%02u\n"
		"Level:   %lu l\n"
		"Press 1: %lu.%02lu MPa\n",
//		"Press 2: %d.%02d MPa\n",
#if LOGGER_DEBUG
		log_record.id,
#endif
		log_record.time[0], log_record.time[1], log_record.time[2], log_record.time[3], log_record.time[4], log_record.time[5],
		log_record.level,
		log_record.press_1 / 100, log_record.press_1 % 100
//		cur_log_record.press_2 / 100, cur_log_record.press_2 % 100
	);
}

void _parse_response()
{
	char* var_ptr = get_response();
	char* data_ptr = var_ptr;
	if (!var_ptr) {
		goto do_error;
	}
	// Parse time
	DateTime datetime = {0};

	data_ptr = strnstr(var_ptr, TIME_FIELD, strlen(var_ptr));
	if (!data_ptr) {
		goto do_error;
	}
	data_ptr += strlen(TIME_FIELD);
	datetime.year = atoi(data_ptr) % 100;

	data_ptr = strnstr(data_ptr, T_DASH_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		goto do_error;
	}
	data_ptr += strlen(T_DASH_FIELD);
	datetime.month = (uint8_t)atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_DASH_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		goto do_error;
	}
	data_ptr += strlen(T_DASH_FIELD);
	datetime.date = atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_TIME_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		goto do_error;
	}
	data_ptr += strlen(T_TIME_FIELD);
	datetime.hour = atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_COLON_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		goto do_error;
	}
	data_ptr += strlen(T_COLON_FIELD);
	datetime.minute = atoi(data_ptr);

	data_ptr = strnstr(data_ptr, T_COLON_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		goto do_error;
	}
	data_ptr += strlen(T_COLON_FIELD);
	datetime.second = atoi(data_ptr);

    if(!save_datetime(&datetime)) {
#if LOGGER_DEBUG
    	LOG_DEBUG(LOG_TAG, "parse error - unable to update datetime\n");
#endif
		goto do_error;
    }

#if LOGGER_DEBUG
	LOG_DEBUG(LOG_TAG, "time updated\n");
#endif

	if (module_settings.server_log_id < sended_log_id) {
		module_settings.pump_work_sec = 0;
		module_settings.pump_downtime_sec = 0;
		module_settings.server_log_id = sended_log_id;
		settings_save();
#if LOGGER_DEBUG
		LOG_DEBUG(LOG_TAG, "server log id updated\n");
#endif
	}

	// Parse configuration:
	data_ptr = var_ptr;
	data_ptr = strnstr(data_ptr, CF_LOGID_FIELD, strlen(data_ptr));
	if (data_ptr) {
		module_settings.server_log_id = atoi(data_ptr + strlen(CF_LOGID_FIELD));
	}

	LOG_MESSAGE(LOG_TAG, "Recieved response from the server\n");

	data_ptr = var_ptr;
	data_ptr = strnstr(data_ptr, CF_ID_FIELD, strlen(data_ptr));
	if (!data_ptr) {
		goto do_exit;
	}
	uint32_t new_cf_id = atoi(data_ptr + strlen(CF_ID_FIELD));
	if (new_cf_id == module_settings.cf_id) {
		goto do_exit;
	}
	module_settings.cf_id = new_cf_id;

	data_ptr = strnstr(var_ptr, CF_DATA_FIELD, strlen(var_ptr));
	if (!data_ptr) {
		goto do_error;
	}
	data_ptr += strlen(CF_DATA_FIELD);

	var_ptr = data_ptr;
	data_ptr = strnstr(var_ptr, CF_PWR_FIELD, strlen(var_ptr));
	if (data_ptr) {
		bool enabled = atoi(data_ptr + strlen(CF_PWR_FIELD));
		module_settings.pump_enabled = enabled;
		pump_update_enable_state(enabled);
	}

	data_ptr = strnstr(var_ptr, CF_LTRMIN_FIELD, strlen(var_ptr));
	if (data_ptr) {
		module_settings.tank_liters_min = atoi(data_ptr + strlen(CF_LTRMIN_FIELD));
	}

	data_ptr = strnstr(var_ptr, CF_LTRMAX_FIELD, strlen(var_ptr));
	if (data_ptr) {
		module_settings.tank_liters_max = atoi(data_ptr + strlen(CF_LTRMAX_FIELD));
	}

	data_ptr = strnstr(var_ptr, CF_TRGT_FIELD, strlen(var_ptr));
	if (data_ptr) {
		module_settings.milliliters_per_day = atoi(data_ptr + strlen(CF_TRGT_FIELD));
	}

	data_ptr = strnstr(var_ptr, CF_SLEEP_FIELD, strlen(var_ptr));
	if (data_ptr) {
		update_log_sleep(atoi(data_ptr + strlen(CF_SLEEP_FIELD)) * MILLIS_IN_SECOND);
	}

	data_ptr = strnstr(var_ptr, CF_SPEED_FIELD, strlen(var_ptr));
	if (data_ptr) {
		pump_update_speed(atoi(data_ptr + strlen(CF_SPEED_FIELD)));
	}

	data_ptr = strnstr(var_ptr, CF_MOD_ID_FIELD, strlen(var_ptr));
	if (data_ptr) {
		module_settings.id = atoi(data_ptr + strlen(CF_MOD_ID_FIELD));
	}

	data_ptr = strnstr(var_ptr, CF_CLEAR_FIELD, strlen(var_ptr));
	if (data_ptr && atoi(data_ptr + strlen(CF_CLEAR_FIELD)) == 1) {
		clear_log();
	}

#if LOGGER_DEBUG
	LOG_DEBUG(LOG_TAG, "configuration updated\n");
#endif
	show_settings();

	goto do_success;


do_error:
#if LOGGER_DEBUG
	LOG_DEBUG(LOG_TAG, "unable to parse response - %s\n", get_response());
#endif
	goto do_exit;

do_success:
	if (settings_save() == SETTINGS_OK) {
		LOG_MESSAGE(LOG_TAG, "New settings have been received from the server\n");
	} else {
		LOG_MESSAGE(LOG_TAG, "Unable to recieve new settings from the server\n");
		module_settings.cf_id = 0;
	}
	goto do_exit;

do_exit:
	return;
}
