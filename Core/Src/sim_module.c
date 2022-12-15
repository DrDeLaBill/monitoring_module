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


#define LINE_BREAK_COUNT 2
#define UART_TIMEOUT     100
#define END_OF_STRING    0x1a
#define REQUEST_SIZE     250
#define RESPONSE_SIZE    80
#define UART_WAIT        10000
#define RESTART_WAIT     1000


//uint32_t _reset_time = 0;
//bool _sim_module_initialized = false;
//bool _sim_module_ready = false;
//bool _GPRS_ready = false;
//uint8_t http_response[RESP_BUFFER_HTTP_SIZE] = {},
//		AT_command[REQUEST_COMMAND_SIZE] = {},
//		request[REQUEST_COMMAND_SIZE] = {},
//		recieve_buffer[RESP_BUFFER_SIZE] = {};
//bool _server_available = false;

//bool _start_http();
//void _stop_http();
//bool _try_to_chttpact(char* url, char* data);
//bool _reset_sim_module();
//bool _check_sim_model();
//bool _start_GPRS();
//void _reset_sim_module_state();
//void _clear_response();

void _check_module_response();
void _send_config_command();
void _check_http_response();
void _shift_response();
void _send_AT_command(const char* command);
void _reset_module();
void _start_module();


struct _sim_module_state {
	dio_timer_t uart_wait;
	dio_timer_t restart_wait;
	bool sim_module_ready;
	bool gprs_ready;
	bool wait_module_response;
	bool wait_http_resoponse;
	bool error;
} sim_state;

const char* SIM_TAG = "SIM";
const char* SUCCESS_RESPONSE = "OK";
const char** sim_config_list = {
	"AT",
	"ATE0",
	"AT+CPIN?",
	NULL
};
const char** gprs_config_list = {
	"AT+CGDCONT=1,\"IP\",\"internet\",\"0.0.0.0\"",
	"AT+CGSOCKCONT=1,\"IP\",\"internet\"",
	"AT+CSOCKSETPN=1",
	"AT+CSOCKSETPN=0",
	NULL
};
char* response[RESPONSE_SIZE] = {};
char* http_response[RESPONSE_SIZE] = {};


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	static line_break_counter = 0;

	if (strlen(response) > 0) {
		line_break_counter = response[strlen(response)-1] == '\n' ? line_break_counter + 1 : 0;
	}

	if (line_break_counter > LINE_BREAK_COUNT) {
		_clear_response();
		HAL_UART_Receive(&SIM_MODULE_UART, http_response, RESPONSE_SIZE, UART_TIMEOUT);
		LOG_DEBUG(SIM_TAG, "HTTP response: %s\r\n", http_response);
		line_break_counter = 0;
	}

	if (strlen(response) >= RESPONSE_SIZE) {
		_shift_response();
	}

	HAL_UART_Receive_IT(&SIM_MODULE_UART, response + strlen(response), sizeof(char));
}

void sim_module_begin() {
	memset(sim_state, 0, suzeof(sim_state));
	Util_TimerStart(sim_state.uart_wait, UART_WAIT);
	Util_TimerStart(sim_state.restart_wait, RESTART_WAIT);
	_start_module();
	HAL_UART_Receive_IT(&SIM_MODULE_UART, response + strlen(response), sizeof(char));
}

void sim_module_proccess()
{
	if (sim_state.error && !Util_TimerPending(sim_state.restart_wait)) {
		_reset_module();
		sim_state.waiting_time = HAL_GetTick();
	}

	if (!sim_state.error && !Util_TimerPending(sim_state.restart_wait)) {
		_start_module();
		sim_state.error = false;
	}

	if (sim_state.error) {
		return;
	}

	if (sim_state.wait_module_response) {
		_check_module_response();
	} else {
		_send_config_command();
	}
}

void send_http(const char* url, const char* data)
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

	char* request[REQUEST_SIZE] = {};
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
	LOG_DEBUG(SIM_TAG, "%s\r\n", request);
	HAL_UART_Transmit(_sim_huart, (uint8_t *)request, strlen(request), UART_TIMEOUT);
}

char* get_response()
{
	return http_response;
}

void _check_module_response()
{
	if (!Util_TimerPending(sim_state.uart_wait)) {
		sim_state.wait_module_response = false;
		sim_state.error = true;
		_clear_response();
		LOG_DEBUG(SIM_TAG, "module error\r\n");
	}

	if (strstr(response, SUCCESS_RESPONSE)) {
		sim_state.wait_module_response = false;
		HAL_UART_Receive(&SIM_MODULE_UART, response, RESPONSE_SIZE, UART_TIMEOUT);
		_clear_response();
	}

	if (strlen(response) >= RESPONSE_SIZE - 1) {
		_shift_response();
	}
}

void _send_config_command()
{
	static char** config_pos = sim_config_list;
	static char** gprs_pos = gprs_config_list;
	if (sim_state.error || !Util_TimerPending(sim_state.uart_wait)) {
		sim_state.error = true;
		config_pos = sim_config_list;
		gprs_pos = gprs_config_list;
	}

	if (!sim_state.sim_module_ready && *config_pos) {
		_send_AT_command(*config_pos);
		sim_state.wait_module_response = true;
		config_pos = *config_pos ? config_pos + 1 : config_pos;
	} else {
		sim_state.sim_module_ready = true;
		_send_AT_command(*gprs_pos);
		sim_state.wait_module_response = true;
		gprs_pos = *gprs_pos ? gprs_pos + 1 : gprs_pos;
	}

	if (sim_state.sim_module_ready && !*config_pos && !*gprs_pos) {
		sim_state.gprs_ready = true;
		return;
	}

	Util_TimerStart(sim_state.uart_wait, UART_WAIT);
}

void _send_AT_command(const char* command)
{
	LOG_DEBUG(SIM_TAG, "Command: %s\r\n", command);
	HAL_UART_Transmit(&SIM_MODULE_UART, command, strlen(command), UART_TIMEOUT);
}

void _shift_response() {
	strncpy(response, response + RESPONSE_SIZE / 2, RESPONSE_SIZE);
	memset(response + RESPONSE_SIZE / 2, 0, RESPONSE_SIZE / 2);
}

void _clear_response()
{
	memset(response, 0, sizeof(response));
}

void _resеt_module()
{
	HAL_GPIO_WritePin(_reset_GPIO_port, _reset_pin, GPIO_PIN_RESET);
}

void _start_module()
{
	HAL_GPIO_WritePin(_reset_GPIO_port, _reset_pin, GPIO_PIN_SET);
}







bool send_log()
{
	char data[REQUEST_JSON_SIZE] = {};
	if (!INA3221_available()) {
		send_debug_message("INA3221 sensor is not connected");
	}
	snprintf(
		data,
		"\"id\": %d,"
		"\"time\": \"%s\","
		"\"first_shunt\": %.2f,"
		"\"second_shint\": %.2f,"
		"\"liquid_level\": %.2f",
		module_settings.id,
		get_time_string(),
		INA3221_getCurrent_mA(1),
		INA3221_getCurrent_mA(2),
		get_liquid_in_liters()
	);
	send_debug_message("Log data:");
	send_debug_message(data);

	if (!write_record(data)) {
		send_debug_message("ERROR WRITE LOG TO SD CARD");
	}
	if (!send_http_POST("upload-log", data)) {
		goto fail;
	}
	if (!strstr(http_response, "200 ok")) {
	}
	send_debug_message("ERROR SEND LOG");

success:
	send_debug_message("SEND LOG SUCCESS");
	_server_available = true;
	return true;

fail:
	_server_available = false;
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
