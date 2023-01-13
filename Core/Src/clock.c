/*
 * clock.c
 *
 *  Created on: 11 янв. 2023 г.
 *      Author: georg
 */


#include "clock.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "settings.h"
#include "sim_module.h"
#include "settings_manager.h"
#include "ds1307_for_stm32_hal.h"


#define REQUEST_SIZE 110


void _send_time_request();
void _set_response_time(const char* response);


const char* CLOCK_TAG = "CLCK";
const char* TIME_RESP_FIELD = "time=";


void clock_proccess()
{
	if (module_settings.is_time_recieved) {
		return;
	}

	if (is_module_ready() && !is_server_available()) {
		connect_to_server();
	}

	if (is_server_available()) {
		_send_time_request();
	}

	if (is_http_success()) {
		_set_response_time(get_response());
	}
}

void _send_time_request()
{
	if (is_http_busy()) {
		return;
	}

	char data[REQUEST_SIZE] = {};
	snprintf(
		data,
		sizeof(data),
		"GET /api/v1/get-time HTTP/1.1\r\n"
		"Host: %s:%s\r\n"
		"Content-Type: text/plain\r\n"
		"%c\r\n",
		module_settings.server_url,
		module_settings.server_port,
		END_OF_STRING
	);

	send_http(data);
}

void _set_response_time(const char* response)
{
	LOG_DEBUG(CLOCK_TAG, " SERVER RESPONSE\r\n%s\r\n", response);
	char *time_ptr = strstr(response, TIME_RESP_FIELD) + strlen(TIME_RESP_FIELD) + 1;

	DS1307_SetYear(atoi(time_ptr));
	time_ptr = strstr(time_ptr, "-") + 1;
	DS1307_SetMonth(atoi(time_ptr));
	time_ptr = strstr(time_ptr, "-") + 1;
	DS1307_SetDate(atoi(time_ptr));
	time_ptr = strstr(time_ptr, "T") + 1;
	DS1307_SetHour(atoi(time_ptr));
	time_ptr = strstr(time_ptr, ":") + 1;
	DS1307_SetMinute(atoi(time_ptr));
	time_ptr = strstr(time_ptr, ":") + 1;
	DS1307_SetSecond(atoi(time_ptr));

	module_settings.is_time_recieved = true;
	settings_save();
}
