/*
 * sim_module.c
 *
 *  Created on: Sep 4, 2022
 *      Author: georg
 */

#include "sim_module.h"

#include "settings.h"
#include "clock.h"
#include "sd_card_log.h"
#include "ina3221_sensor.h"
#include "liquid_sensor.h"
#include "sd_card_log.h"

UART_HandleTypeDef *_sim_huart;
GPIO_TypeDef* _reset_GPIO_port;
uint16_t _reset_pin;
uint32_t _reset_time = 0;
bool _sim_module_initialized = false;
bool _sim_module_ready = false;
bool _GPRS_ready = false;
bool _server_available = false;
uint8_t http_response[RESP_BUFFER_HTTP_SIZE] = {},
		AT_command[REQUEST_COMMAND_SIZE] = {},
		request[REQUEST_COMMAND_SIZE] = {},
		recieve_buffer[RESP_BUFFER_SIZE] = {};

bool _start_http();
void _stop_http();
bool _try_to_chttpact(char* url, char* data);
bool _reset_sim_module();
bool _send_AT_command(char* command, uint16_t response_timeout);
bool _check_sim_model();
bool _start_GPRS();
void _reset_sim_module_state();
void _clear_response();

void sim_module_begin(UART_HandleTypeDef* sim_huart, GPIO_TypeDef* sim_reset_GPIO_port, uint16_t sim_reset_pin) {
	_sim_huart = sim_huart;
	_reset_GPIO_port = sim_reset_GPIO_port;
	_reset_pin = sim_reset_pin;
	if (!_reset_sim_module()) {
		return;
	}
	if (!sim_module_init()) {
		return;
	}
	_sim_module_initialized = true;
	send_debug_message("SIM MODULE STARTED");
}

void restart_sim_module() {
	_reset_sim_module();
}

bool send_log()
{
	char data[REQUEST_JSON_SIZE-1] = {};
	if (!INA3221_available()) {
		send_debug_message("INA3221 sensor is not connected");
	}
	sprintf(
		data,
		"{"
			"\"id\": %d,"
			"\"time\": \"%s\","
			"\"first_shunt\": %.2f,"
			"\"second_shint\": %.2f,"
			"\"liquid_level\": %.2f"
		"}\n",
		module_settings.id,
		get_time_string(),
		INA3221_getCurrent_mA(1),
		INA3221_getCurrent_mA(2),
		get_liquid_in_liters()
	);
	send_debug_message("Log data:");
	send_debug_message(data);
	if (!send_http_POST("upload-log", data)) {
		_server_available = false;
	}
	if (strstr(http_response, "200 ok")) {
		send_debug_message("SEND LOG SUCCESS");
		_server_available = true;
		return true;
	}
	send_debug_message("ERROR SEND LOG");
	if (!write_record(data)) {
		send_debug_message("ERROR WRITE LOG TO SD CARD");
		_server_available = false;
		return false;
	}
	send_debug_message("WRITE LOG TO SD CARD SUCCESS");
	return false;
}

bool send_old_log()
{
	if (!is_server_available()) {
		return false;
	}
	char data[REQUEST_JSON_SIZE+1] = {};
	strcpy(data, get_first_old_log());
	if (!send_http_POST("upload-log", data)) {
		_server_available = false;
		return false;
	}
	if (!strstr(http_response, "200 ok")) {
		return false;
	}
	send_debug_message("SEND OLD LOG SUCCESS");
	_server_available = true;
	if (!remove_first_old_log()) {
		send_debug_message("ERROR SD CARD: unable to remove old log");
		return false;
	}
	return true;
}

bool is_server_available()
{
	return _server_available;
}

char*  send_http_GET(char* url) {
	_clear_response();
	if (!_start_http()) {
		_reset_sim_module_state();
		return "";
	}

	sprintf(AT_command, "AT+CHTTPACT=\"%s\",%s\r\n", module_settings.server_url, module_settings.server_port);
	send_debug_message("Command:");
	send_debug_message(AT_command);
	HAL_UART_Transmit(_sim_huart, (uint8_t *)AT_command, strlen(AT_command), DEFAULT_AT_TIMEOUT);
	HAL_UART_Receive(_sim_huart, http_response, RESP_BUFFER_HTTP_SIZE, DEFAULT_HTTP_TIMEOUT);
	send_debug_message("Response:");
	send_debug_message(http_response);
	if (!strstr(http_response, "+CHTTPACT: REQUEST")) {
		_reset_sim_module_state();
		return "";
	}

	sprintf(
		request,
		"GET /%s HTTP/1.1\r\n"
		"Host: %s:%s\r\n"
		"User-Agent: SIM5320E\r\n"
		"Content-Length: 0\r\n\r\n"
		"%c\r\n",
		url,
		module_settings.server_url,
		module_settings.server_port,
		END_OF_STRING
	);
	send_debug_message("Command:");
	send_debug_message(request);
	HAL_UART_Transmit(_sim_huart, (uint8_t *)request, strlen(request), DEFAULT_AT_TIMEOUT);
	HAL_UART_Receive(_sim_huart, http_response, RESP_BUFFER_HTTP_SIZE, DEFAULT_HTTP_TIMEOUT);
	send_debug_message("Response:");
	send_debug_message(http_response);
	if (!strstr(http_response, "+CHTTPACT: 0")) {
		_reset_sim_module_state();
		return "";
	}

	_stop_http();

	_server_available = true;
	return http_response;
}

bool send_http_POST(char* url, char* data)
{
	if (!strlen(data)) {
		send_debug_message("Empty POST data");
		return false;
	}

	_clear_response();
	if (!_start_http(url)) {
		_reset_sim_module_state();
		return false;
	}

	bool is_server_connected = false;
	for (uint8_t i = 0; i < NUMBER_OF_ACT_ATTEMPTS && !is_server_connected; i++) {
		is_server_connected = _try_to_chttpact(url, data);
	}

	if (!is_server_connected) {
		return false;
	}

    _stop_http();

    return true;
}

bool _start_http()
{
	if (!_sim_module_initialized) {
		_reset_sim_module_state();
		return false;
	}
	if (!_send_AT_command("AT+CHTTPSSTART", LONG_AT_TIMEOUT)) {
		_reset_sim_module_state();
		return false;
	}
	return true;
}

void _stop_http()
{
    _send_AT_command("AT+CHTTPSSTOP", LONG_AT_TIMEOUT);
}

bool _try_to_chttpact(char* url, char* data)
{
	sprintf(AT_command, "AT+CHTTPACT=\"%s\",%s\r\n", module_settings.server_url, module_settings.server_port);
	send_debug_message("Command:");
	send_debug_message(AT_command);
	HAL_UART_Transmit(_sim_huart, (uint8_t *)AT_command, strlen(AT_command), DEFAULT_AT_TIMEOUT);
	HAL_UART_Receive(_sim_huart, http_response, RESP_BUFFER_HTTP_SIZE, LONG_AT_TIMEOUT);
	send_debug_message("Response:");
	send_debug_message(http_response);
	if (!strstr(http_response, "+CHTTPACT: REQUEST")) {
		return false;
	}

	sprintf(
		request,
		"POST /%s HTTP/1.1\r\n"
		"Host: %s:%s\r\n"
		"User-Agent: SIM5320E\r\n"
		"Accept: */*\r\n"
		"Content-Type: application/json\r\n"
		"Cache-Control: no-cache\r\n"
		"Accept-Charset: utf-8, us-ascii\r\n"
		"Pragma: no-cache\r\n"
		"Content-Length: %d\r\n\r\n"
		"%s\r\n"
		"%c\r\n",
		url,
		module_settings.server_url,
		module_settings.server_port,
		strlen(data),
		data,
		END_OF_STRING
	);
	send_debug_message("Command:");
	send_debug_message(request);
	HAL_UART_Transmit(_sim_huart, (uint8_t *)request, strlen(request), DEFAULT_AT_TIMEOUT);
	HAL_UART_Receive(_sim_huart, http_response, RESP_BUFFER_HTTP_SIZE, DEFAULT_HTTP_TIMEOUT);
	send_debug_message("Response:");
	send_debug_message(http_response);
	if (strstr(http_response, "+CHTTPACT: 0")) {
		return true;
	}

	return false;
}

bool is_sim_module_ready() {
	return _sim_module_ready;
}

bool is_GPRS_ready() {
	return _GPRS_ready;
}

bool _reset_sim_module()
{
	_reset_sim_module_state();
	if (abs(HAL_GetTick() - _reset_time) < RESET_DELAY) {
		char end_of_message = END_OF_STRING;
		HAL_UART_Transmit(_sim_huart, &end_of_message, 1, DEFAULT_AT_TIMEOUT);
		return true;
	}
	_reset_time = HAL_GetTick();
	HAL_GPIO_WritePin(_reset_GPIO_port, _reset_pin, GPIO_PIN_RESET);
	HAL_Delay(RESET_DURATION);
	HAL_GPIO_WritePin(_reset_GPIO_port, _reset_pin, GPIO_PIN_SET);
	_stop_http();
	send_debug_message("RESTARTING SIM MODULE");
	HAL_Delay(LONG_AT_TIMEOUT);
	sim_module_init();
	return true;
}

bool sim_module_init()
{
	if (!_send_AT_command("AT", DEFAULT_AT_TIMEOUT)) {
		_reset_sim_module_state();
		return false;
	}
	if (!_send_AT_command("ATE0", DEFAULT_AT_TIMEOUT)) {
		_reset_sim_module_state();
		return false;
	}
	if (!_check_sim_model()) {
		_reset_sim_module_state();
		return false;
	}
	if (!_send_AT_command("AT+CPIN?", DEFAULT_AT_TIMEOUT)) {
		_reset_sim_module_state();
		return false;
	}

	_sim_module_ready = true;
	return _start_GPRS();
}

bool _send_AT_command(char* command, uint16_t response_timeout)
{
	sprintf(AT_command, "%s\r\n", command);
	HAL_UART_Transmit(_sim_huart, (uint8_t *)AT_command, strlen(AT_command), DEFAULT_AT_TIMEOUT);
	HAL_UART_Receive(_sim_huart, recieve_buffer, RESP_BUFFER_SIZE, response_timeout);

	bool status = (bool)strstr(recieve_buffer, "OK");
	memset(recieve_buffer, 0, sizeof(recieve_buffer));
	return status;
}

bool _check_sim_model()
{
	strcpy(AT_command, "AT+GMM\r\n");
	HAL_UART_Transmit(_sim_huart, (uint8_t *)AT_command, strlen(AT_command), DEFAULT_AT_TIMEOUT);
	HAL_UART_Receive(_sim_huart, recieve_buffer, RESP_BUFFER_SIZE, DEFAULT_AT_TIMEOUT);

	bool status = (bool)strstr(recieve_buffer, SIM_MODULE_MODEL);
	memset(recieve_buffer, 0, sizeof(recieve_buffer));
	return status;
}

bool _start_GPRS()
{
    if (!_send_AT_command("AT+CGDCONT=1,\"IP\",\"internet\",\"0.0.0.0\"", DEFAULT_AT_TIMEOUT)) {
		_reset_sim_module_state();
		return false;
	}
    if (!_send_AT_command("AT+CGSOCKCONT=1,\"IP\",\"internet\"", DEFAULT_AT_TIMEOUT)) {
		_reset_sim_module_state();
    	return false;
    }
    if (!_send_AT_command("AT+CSOCKSETPN=1", DEFAULT_AT_TIMEOUT)) {
		_reset_sim_module_state();
		_send_AT_command("AT+CSOCKSETPN=0", DEFAULT_AT_TIMEOUT);
    	return false;
    }

    _GPRS_ready = true;
    return true;
}

void _reset_sim_module_state()
{
	if (_sim_module_ready || _GPRS_ready) {
		_stop_http();
	}
	_sim_module_initialized = false;
	_sim_module_ready = false;
	_server_available = false;
	_GPRS_ready = false;
}

bool is_sim_module_initialized()
{
	return _sim_module_initialized;
}

void _clear_response()
{
	memset(http_response, 0, sizeof(http_response));
	memset(recieve_buffer, 0, sizeof(recieve_buffer));
	memset(request, 0, sizeof(request));
}
