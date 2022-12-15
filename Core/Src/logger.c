/*
 * data_logger.c
 *
 *  Created on: Dec 15, 2022
 *      Author: RCPark
 */


#include "logger.h"

#include "stm32f1xx_hal.h"

#include "sim_module.h"
#include "settings.h"
#include "utils.h"

#define LOG_SIZE 250


void _write_log();
void _remove_old_log();
void _get_old_log();
void _check_and_remove(const char* response);


const char* LOG_TAG = "LOG";

dio_timer_t log_timer;


void logger_begin()
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
			"pussy"
		);
		send_log(data);
	}

	if (is_http_success()) {
		LOG_DEBUG(LOG_TAG, "%s\r\n", get_response());
		_check_and_remove(get_response());
	}

	if (Util_TimerPending(&log_timer)) {
		return;
	}

	_remove_old_log();
	_write_log();
}

void _check_and_remove(const char* response)
{

}

void _write_log()
{

}

void _remove_old_log()
{

}

void _get_old_log()
{

}
