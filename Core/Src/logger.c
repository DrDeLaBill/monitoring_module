/*
 * record.c
 *
 *  Created on: 19 дек. 2022 г.
 *      Author: georgy
 */

#include "logger.h"

#include "defines.h"
#include "utils.h"
#include "liquid_sensor.h"
#include "record_manager.h"
#include "sim_module.h"
#include "settings.h"


#define LOG_SIZE 250


void _general_record_save(record_sd_payload_t* payload);
void _general_record_load(const record_sd_payload_t* payload);


const char *LOG_TAG = "LOG";


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
	payload->v1.payload_record.id      = get_new_id();
	payload->v1.payload_record.cf_id   = CF_VERSION;
	payload->v1.payload_record.fw_id   = FW_VERSION;
	payload->v1.payload_record.level   = get_liquid_level();
	payload->v1.payload_record.press_1 = INA3221_getCurrent_mA(1);
	payload->v1.payload_record.press_2 = INA3221_getCurrent_mA(2);
	snprintf(
		payload->v1.payload_record.time,
		sizeof(payload->v1.payload_record.time),
		"%d-%02d-%02dT%02d:%02d:%02d",
		DS1307_GetYear(),
		DS1307_GetMonth(),
		DS1307_GetDate(),
		DS1307_GetHour(),
		DS1307_GetMinute(),
		DS1307_GetSecond()
	);
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
	Util_TimerStart(&log_timer, module_settings.sleep_time);
}

void logger_proccess()
{
	if (is_module_ready() && !is_server_available()) {
		connect_to_server();
	}

	if (is_server_available()) {
		char data[LOG_SIZE] = {};
		snprintf(
			data,
			"pussy",
			12
		);
		send_http(data);
	}

	if (is_http_success()) {
		LOG_DEBUG(LOG_TAG, "%s\r\n", get_response());
//		_check_and_remove(get_response());
	}

	if (Util_TimerPending(&log_timer)) {
		return;
	}

//	_remove_old_log();
//	_write_log();
}
