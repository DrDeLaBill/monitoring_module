/*
 * record.c
 *
 *  Created on: 19 дек. 2022 г.
 *      Author: georgy
 */

#include "logger.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "defines.h"
#include "utils.h"
#include "liquid_sensor.h"
#include "record_manager.h"
#include "sim_module.h"
#include "settings.h"
#include "ds1307_for_stm32_hal.h"
#include "pressure_sensor.h"


void _general_record_save(record_sd_payload_t* payload);
void _general_record_load(const record_sd_payload_t* payload);
void _send_http_log();
void _make_measurements();
void _show_measurements();
void _parse_response();
void _timer_start();


const char* LOG_TAG         = "LOG";

const char* DATA_FIELD      = "\r\n\r\nd=";
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
const char* CF_LOGID_FIELD  = "logid=";


LogRecord log_record;
record_tag_t general_record_load = {
	.save_cb = &_general_record_save,
	.load_cb = &_general_record_load
};
record_tag_t* record_cbs[] = {
	&general_record_load,
	NULL
};
dio_timer_t log_timer;


void _general_record_save(record_sd_payload_t* payload)
{
	payload->v1.payload_record.id      = log_record.id;
	payload->v1.payload_record.cf_id   = log_record.cf_id;
	payload->v1.payload_record.fw_id   = log_record.fw_id;
	payload->v1.payload_record.level   = log_record.level;
	payload->v1.payload_record.press_1 = log_record.press_1;
	payload->v1.payload_record.press_2 = log_record.press_2;
	strncpy(payload->v1.payload_record.time, log_record.time, sizeof(log_record.time));
}

void _general_record_load(const record_sd_payload_t* payload)
{
	log_record.id      = payload->v1.payload_record.id;
	log_record.cf_id   = payload->v1.payload_record.cf_id;
	log_record.fw_id   = payload->v1.payload_record.fw_id;
	log_record.level   = payload->v1.payload_record.level;
	log_record.press_1 = payload->v1.payload_record.press_1;
	log_record.press_2 = payload->v1.payload_record.press_2;
	strncpy(log_record.time, payload->v1.payload_record.time, sizeof(log_record.time));
}

void logger_manager_begin()
{
	update_log_timer();
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

	if (Util_TimerPending(&log_timer)) {
		return;
	}

	if (!module_settings.sleep_time) {
		LOG_DEBUG(LOG_TAG, "no setting - sleep_time\r\n");
		module_settings.sleep_time = DEFAULT_SLEEPING_TIME;
		return;
	}

	_make_measurements();
	_show_measurements();
	record_save();
	update_log_timer();
}

void update_log_timer()
{
	Util_TimerStart(&log_timer, module_settings.sleep_time);
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
		"t=%d-%02d-%02dT%02d:%02d:%02d\n",
		module_settings.id,
		log_record.fw_id,
		log_record.cf_id,
		DS1307_GetYear(),
		DS1307_GetMonth(),
		DS1307_GetDate(),
		DS1307_GetHour(),
		DS1307_GetMinute(),
		DS1307_GetSecond()
	);

	record_status_t record_res = next_record_load();
	if (record_res == RECORD_OK) {
		snprintf(
			data + strlen(data),
			sizeof(data) - strlen(data),
			"d="
				"id=%lu;"
				"t=%s;"
				"level=%d.%d;"
				"press_1=%d.%02d;"
				"press_2=%d.%02d;"
				"pump=%lu\n",
			log_record.id,
			log_record.time,
			FLOAT_AS_STRINGS(log_record.level),
			FLOAT_AS_STRINGS(log_record.press_1),
			FLOAT_AS_STRINGS(log_record.press_2),
			module_settings.pump_work_seconds
		);
	}

	send_http_post(data);
}

void _make_measurements()
{
	log_record.fw_id   = FW_VERSION;
	log_record.cf_id   = module_settings.cf_id;
	log_record.id      = get_new_id();
	log_record.level   = get_liquid_liters();
	log_record.press_1 = get_first_press();
	log_record.press_2 = get_second_press();
	snprintf(
		log_record.time,
		sizeof(log_record.time),
		"%d-%02d-%02dT%02d:%02d:%02d",
		DS1307_GetYear(),
		DS1307_GetMonth(),
		DS1307_GetDate(),
		DS1307_GetHour(),
		DS1307_GetMinute(),
		DS1307_GetSecond()
	);
}

void _show_measurements()
{
	LOG_DEBUG(
		LOG_TAG,
		"\r\nID: %lu\r\n"
		"Time: %s\r\n"
		"Level: %d.%d\r\n"
		"Press 1: %d.%d\r\n"
		"Press 2: %d.%d\r\n",
		log_record.id,
		log_record.time,
		FLOAT_AS_STRINGS(log_record.level),
		FLOAT_AS_STRINGS(log_record.press_1),
		FLOAT_AS_STRINGS(log_record.press_2)
	);
}

void _parse_response()
{
	char *var_ptr = get_response();
	// Parse time
	var_ptr = strnstr(var_ptr, TIME_FIELD, strlen(var_ptr));
	if (!var_ptr) {
		goto do_error;
	}
	var_ptr += strlen(TIME_FIELD);
	uint16_t tmp = atoi(var_ptr);
	DS1307_SetYear(tmp);

	var_ptr = strnstr(var_ptr, "-", strlen(var_ptr)) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetMonth((uint8_t)atoi(var_ptr));

	var_ptr = strnstr(var_ptr, "-", strlen(var_ptr)) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetDate(atoi(var_ptr));

	var_ptr = strnstr(var_ptr, "t", strlen(var_ptr)) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetHour(atoi(var_ptr));

	var_ptr = strnstr(var_ptr, ":", strlen(var_ptr)) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetMinute(atoi(var_ptr));

	var_ptr = strnstr(var_ptr, ":", strlen(var_ptr)) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetSecond(atoi(var_ptr));

	module_settings.server_log_id = log_record.id;
	LOG_DEBUG(LOG_TAG, " response recieve success - time updated, server log id updated\n");


	// Parse configuration:
	var_ptr = get_response();
	var_ptr = strnstr(var_ptr, CF_ID_FIELD, strlen(var_ptr)) + strlen(CF_ID_FIELD) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	uint32_t new_cf_id = atoi(var_ptr);
	if (new_cf_id == module_settings.cf_id) {
		goto do_success;
	}

	char *cnfg_ptr = strnstr(var_ptr, CF_DATA_FIELD, strlen(var_ptr)) + strlen(CF_DATA_FIELD) + 1;
	if (!var_ptr) {
		goto do_error;
	}

	var_ptr = strnstr(cnfg_ptr, CF_PWR_FIELD, strlen(cnfg_ptr)) + strlen(CF_PWR_FIELD) + 1;
	if (var_ptr) {
		module_settings.module_enabled = atoi(var_ptr);
	}

	var_ptr = strnstr(cnfg_ptr, CF_LTRMIN_FIELD, strlen(cnfg_ptr)) + strlen(CF_LTRMIN_FIELD) + 1;
	if (var_ptr) {
		module_settings.tank_liters_min = atoi(var_ptr);
	}

	var_ptr = strnstr(cnfg_ptr, CF_LTRMAX_FIELD, strlen(cnfg_ptr)) + strlen(CF_LTRMAX_FIELD) + 1;
	if (var_ptr) {
		module_settings.tank_liters_max = atoi(var_ptr);
	}

	var_ptr = strnstr(cnfg_ptr, CF_TRGT_FIELD, strlen(cnfg_ptr)) + strlen(CF_TRGT_FIELD) + 1;
	if (var_ptr) {
		module_settings.milliliters_per_day = atoi(var_ptr);
	}

	var_ptr = strnstr(cnfg_ptr, CF_SLEEP_FIELD, strlen(cnfg_ptr)) + strlen(CF_SLEEP_FIELD) + 1;
	if (var_ptr) {
		module_settings.sleep_time = atoi(var_ptr);
	}

	var_ptr = strnstr(cnfg_ptr, CF_SPEED_FIELD, strlen(cnfg_ptr)) + strlen(CF_SPEED_FIELD) + 1;
	if (var_ptr) {
		module_settings.pump_speed = atoi(var_ptr);
	}

	var_ptr = strnstr(cnfg_ptr, CF_LOGID_FIELD, strlen(cnfg_ptr)) + strlen(CF_LOGID_FIELD) + 1;
	if (var_ptr) {
		module_settings.server_log_id = atoi(var_ptr);
	}

	var_ptr = strnstr(cnfg_ptr, CF_MOD_ID_FIELD, strlen(cnfg_ptr)) + strlen(CF_MOD_ID_FIELD) + 1;
	if (var_ptr) {
		module_settings.id = atoi(var_ptr);
	}

	LOG_DEBUG(LOG_TAG, " configuration updated\n");
	show_settings();

	goto do_success;


do_error:
	LOG_DEBUG(LOG_TAG, " unable to parse response - %s\r\n", get_response());
	goto do_exit;

do_success:
	goto do_exit;

do_exit:
	settings_save();
}
