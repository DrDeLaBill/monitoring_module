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
#include "ina3221_sensor.h"


void _general_record_save(record_sd_payload_t* payload);
void _general_record_load(const record_sd_payload_t* payload);
void _send_http_log();
void _make_measurements();
void _show_measurements();
void _parse_response(const char* response);
void _timer_start();


const char* LOG_TAG       = "LOG";
const char* ID_FIELD      = "id=";
const char* TIME_FIELD    = "t=";
const char* LEVEL_FIELD   = "level=";
const char* PRESS_1_FIELD = "press_1=";
const char* PRESS_2_FIELD = "press_2=";


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
		_parse_response(get_response());
	}

	if (log_timer.delay != module_settings.sleep_time) {
		log_timer.delay = module_settings.sleep_time;
	}

	if (Util_TimerPending(&log_timer)) {
		return;
	}

	if (!module_settings.sleep_time) {
		LOG_DEBUG(LOG_TAG, "no setting - sleep_time\r\n");
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
	record_status_t res = next_record_load();
	if (res != RECORD_OK) {
		LOG_DEBUG(LOG_TAG, "error next_record_load()\r\n");
		return;
	}

	char data[LOG_SIZE] = {};
	snprintf(
		data,
		sizeof(data),
		"POST /api/v1/send HTTP/1.1\r\n"
		"Host: %s:%s\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"fw_id=%lu;"
		"cf_id=%lu;"
		"id=%lu;"
		"d="
			"id=%lu;"
			"t=%s;"
			"level=%.2f;"
			"press_1=%.2f;"
			"press_2=%.2f;"
			"pump=%lu\r\n"
		"%c\r\n",
		module_settings.server_url,
		module_settings.server_port,
		log_record.fw_id,
		log_record.cf_id,
		module_settings.id,
		log_record.id,
		log_record.time,
		log_record.level,
		log_record.press_1,
		log_record.press_2,
		module_settings.pump_work_seconds,
		END_OF_STRING
	);
	send_http(data);
}

void _make_measurements()
{
	log_record.fw_id   = CF_VERSION;
	log_record.cf_id   = FW_VERSION;
	log_record.id      = get_new_id();
	log_record.level   = get_liquid_level();
	log_record.press_1 = INA3221_getCurrent_mA(1);
	log_record.press_2 = INA3221_getCurrent_mA(2);
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
		"Level: %.2f\r\n"
		"Press 1: %.2f\r\n"
		"Press 2: %.2f\r\n",
		log_record.id,
		log_record.time,
		log_record.level,
		log_record.press_1,
		log_record.press_2
	);
}

void _parse_response(const char* response)
{
	LOG_DEBUG(LOG_TAG, " SERVER RESPONSE\r\n%s\r\n", response);

	uint32_t old_id = log_record.id;

	char *var_ptr = strstr(response, ID_FIELD) + strlen(ID_FIELD) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	log_record.id = atoi(var_ptr);

	var_ptr = strstr(response, LEVEL_FIELD) + strlen(LEVEL_FIELD) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	log_record.level = atof(var_ptr);

	var_ptr = strstr(response, PRESS_1_FIELD) + strlen(PRESS_1_FIELD) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	log_record.press_1 = atof(var_ptr);

	var_ptr = strstr(response, PRESS_2_FIELD) + strlen(PRESS_2_FIELD) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	log_record.press_2 = atof(var_ptr);

	record_change(old_id);

	var_ptr = strstr(response, TIME_FIELD) + strlen(TIME_FIELD) + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetYear(atoi(var_ptr));

	var_ptr = strstr(var_ptr, "-") + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetMonth(atoi(var_ptr));

	var_ptr = strstr(var_ptr, "-") + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetDate(atoi(var_ptr));

	var_ptr = strstr(var_ptr, "T") + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetHour(atoi(var_ptr));

	var_ptr = strstr(var_ptr, ":") + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetMinute(atoi(var_ptr));
	if (!var_ptr) {
		goto do_error;
	}

	var_ptr = strstr(var_ptr, ":") + 1;
	if (!var_ptr) {
		goto do_error;
	}
	DS1307_SetSecond(atoi(var_ptr));

	goto do_success;


do_error:
	LOG_DEBUG(LOG_TAG, " unable to parse response - %s\r\n", response);

do_success:
	module_settings.server_log_id = log_record.id;
	module_settings.is_time_recieved = true;
	settings_save();
}
