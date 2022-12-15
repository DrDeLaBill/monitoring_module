/*
 * sim_module.c
 *
 *  Created on: Sep 4, 2022
 *      Author: georg
 */

#include "sim_module.h"

#include "settings.h"
#include "utils.h"
#include "ina3221_sensor.h"
#include "liquid_sensor.h"


#define LINE_BREAK_COUNT 2
#define UART_TIMEOUT     100
#define END_OF_STRING    0x1a
#define REQUEST_SIZE     250
#define RESPONSE_SIZE    80
#define UART_WAIT        10000
#define RESTART_WAIT     1000
#define CONF_CMD_SIZE    9
#define GPRS_CMD_SIZE    40
// SIM module states
#define RESET_STATE      0x00
#define MODULE_READY     0x01
#define GPRS_READY       0x02
#define WAIT_MODULE      0x04
#define WAIT_HTTP        0x08
#define CMD_SUCCESS      0x10
#define HTTP_SUCCESS     0x20
#define ERROR_STATE      0x80


void _check_module_response();
void _send_config_command();
void _shift_response();
void _send_AT_command(const char* command);
void _reset_module();
void _start_module();
uint8_t _update_line_counter(uint8_t *line_break_counter);
bool _if_module_ready();
bool _if_gprs_ready();
bool _if_wait_module();
bool _if_wait_http();
bool _if_cmd_success();
bool _if_http_success();
bool _if_has_error();


struct _sim_module_state {
	dio_timer_t uart_timer;
	dio_timer_t restart_timer;
	uint8_t state;
} sim_state;

const char* SIM_TAG = "SIM";
const char* SUCCESS_CMD_RESP = "OK";
const char* SUCCESS_HTTP_RESP = "200 ok";
const char sim_config_list[][CONF_CMD_SIZE] = {
	{"AT"},
	{"ATE0"},
	{"AT+CPIN?"},
	{""}
};
const char gprs_config_list[][GPRS_CMD_SIZE] = {
	{"AT+CGDCONT=1,\"IP\",\"internet\",\"0.0.0.0\""},
	{"AT+CGSOCKCONT=1,\"IP\",\"internet\""},
	{"AT+CSOCKSETPN=1"},
	{"AT+CSOCKSETPN=0"},
	{""}
};
char* response[RESPONSE_SIZE] = {};
char* http_response[RESPONSE_SIZE] = {};


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	static uint8_t line_break_counter = 0;

	_update_line_counter(&line_break_counter);

	if (strstr(response, SUCCESS_CMD_RESP)) {
		sim_state.state |= CMD_SUCCESS;
		sim_state.state &= ~WAIT_MODULE;
		_clear_response();
	}

	if (strstr(response, SUCCESS_HTTP_RESP)) {
		sim_state.state |= HTTP_SUCCESS;
		sim_state.state &= ~WAIT_HTTP;
		_clear_response();
	}

	if (_if_http_success() && line_break_counter > LINE_BREAK_COUNT) {
		_clear_response();
		HAL_UART_Receive(&SIM_MODULE_UART, http_response, RESPONSE_SIZE, UART_TIMEOUT);
		line_break_counter = 0;
		LOG_DEBUG(SIM_TAG, "HTTP response: %s\r\n", http_response);
	}

	if (strlen(response) >= RESPONSE_SIZE) {
		_shift_response();
	}

	HAL_UART_Receive_IT(&SIM_MODULE_UART, &response[strlen(response)], sizeof(char));
}

void sim_module_begin() {
	memset(&sim_state, 0, sizeof(sim_state));
	Util_TimerStart(&sim_state.uart_timer, UART_WAIT);
	Util_TimerStart(&sim_state.restart_timer, RESTART_WAIT);
	_start_module();
	_clear_response();
	HAL_UART_Receive_IT(&SIM_MODULE_UART, response, sizeof(char));
}

void sim_module_proccess()
{
	if (_if_has_error() && !Util_TimerPending(&sim_state.uart_timer)) {
		_reset_module();
		Util_TimerStart(&sim_state.restart_timer, RESTART_WAIT);
		Util_TimerStart(&sim_state.uart_timer, UART_WAIT);
	}

	if (_if_has_error() && !Util_TimerPending(&sim_state.restart_timer)) {
		_start_module();
		sim_state.state = RESET_STATE;
	}

	if (_if_has_error()) {
		sim_state.state = ERROR_STATE;
		return;
	}

	if (!_if_wait_module()) {
		_send_config_command();
	}
}

void send_http(const char* url, const char* data)
{
	sprintf(AT_command, "AT+CHTTPACT=\"%s\",%s\r\n", module_settings.server_url, module_settings.server_port);
	send_debug_message("Command:");
	send_debug_message(AT_command);
	HAL_UART_Transmit(&SIM_MODULE_UART, (uint8_t *)AT_command, strlen(AT_command), DEFAULT_AT_TIMEOUT);
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

void _send_config_command()
{
	static char** conf_pos = sim_config_list;
	static char** gprs_pos = gprs_config_list;
	if (_if_has_error() || !Util_TimerPending(&sim_state.uart_timer)) {
		sim_state.state |= ERROR_STATE;
		conf_pos = sim_config_list;
		gprs_pos = gprs_config_list;
	}

	if (!_if_module_ready() && strlen(*conf_pos)) {
		_send_AT_command(*conf_pos);
		sim_state.state |= WAIT_MODULE;
		conf_pos++;
	}

	if (_if_module_ready() && strlen(*gprs_pos)) {
		_send_AT_command(*gprs_pos);
		sim_state.state |= WAIT_MODULE;
		gprs_pos++;
	}

	if (_if_module_ready() && !strlen(*conf_pos) && !strlen(*gprs_pos)) {
		sim_state.state |= GPRS_READY;
		sim_state.state &= ~WAIT_MODULE;
		return;
	}

	Util_TimerStart(&sim_state.uart_timer, UART_WAIT);
}

void _send_AT_command(const char* command)
{
	LOG_DEBUG(SIM_TAG, "Command: %s\r\n", command);
	HAL_UART_Transmit(&SIM_MODULE_UART, command, strlen(command), UART_TIMEOUT);
}

uint8_t _update_line_counter(uint8_t *line_break_counter)
{
	if (strlen(response) == 0) {
		return;
	}

	uint8_t pointer = strlen(response) - 1;
	if (response[pointer] == '\r') {
		return;
	}
	if (response[pointer] == '\n') {
		line_break_counter++;
		return;
	}
	line_break_counter = 0;
	return;
}

void _shift_response() {
	strncpy(response, response + RESPONSE_SIZE / 2, RESPONSE_SIZE);
	memset(response + RESPONSE_SIZE / 2, 0, RESPONSE_SIZE / 2);
}

void _clear_response()
{
	memset(response, 0, sizeof(response));
	memset(http_response, 0, sizeof(http_response));
}

void _reset_module()
{
	HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_RESET);
}

void _start_module()
{
	HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_SET);
}

bool _if_module_ready()
{
	return sim_state.state & MODULE_READY;
}

bool _if_gprs_ready()
{
	return sim_state.state & GPRS_READY;
}

bool _if_wait_module()
{
	return sim_state.state & WAIT_MODULE;
}

bool _if_wait_http()
{
	return sim_state.state & WAIT_HTTP;
}

bool _if_cmd_success()
{
	return sim_state.state & CMD_SUCCESS;
}

bool _if_http_success()
{
	return sim_state.state & HTTP_SUCCESS;
}

bool _if_has_error()
{
	return sim_state.state & ERROR_STATE;
}
