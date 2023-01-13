/*
 * sim_module.c
 *
 *  Created on: Sep 4, 2022
 *      Author: georg
 */

#include "sim_module.h"

#include <string.h>
#include <stdlib.h>

#include "settings.h"
#include "utils.h"
#include "ina3221_sensor.h"
#include "liquid_sensor.h"


#define LINE_BREAK_COUNT 2
#define UART_TIMEOUT     100
#define END_OF_STRING    0x1a
#define RESPONSE_SIZE    80
#define UART_WAIT        10000
#define RESTART_WAIT     1000
#define CONF_CMD_SIZE    9
#define GPRS_CMD_SIZE    40
#define HTTP_ACT_SIZE    60
// SIM module states
#define RESET_STATE      0x00
#define MODULE_READY     0b00000001
#define GPRS_READY       0b00000010
#define WAIT_MODULE      0b00000100
#define WAIT_HTTP        0b00001000
#define CMD_SUCCESS      0b00010000
#define HTTP_ACT_SUCCESS 0b00100000
#define HTTP_SUCCESS     0b01000000
#define ERROR_STATE      0b10000000
#define LOAD_SUCCESS	 MODULE_READY | GPRS_READY | HTTP_SUCCESS


void _check_module_response();
void _send_config_command();
void _shift_response();
void _send_AT_command(const char* command);
void _reset_module();
void _start_module();
void _stop_http();
void _clear_response();
void _update_line_counter(uint8_t *line_break_counter);
bool _if_reset_state();
bool _if_module_ready();
bool _if_gprs_ready();
bool _if_wait_module();
bool _if_wait_http();
bool _if_http_act_success();
bool _if_http_success();
bool _if_has_error();
bool _if_load_success();


struct _sim_module_state {
	dio_timer_t uart_timer;
	dio_timer_t restart_timer;
	uint8_t state;
} sim_state;

const char* SIM_TAG = "SIM";
const char* SUCCESS_CMD_RESP = "OK";
const char* SUCCESS_HTTP_ACT = "CHTTPACT: REQUEST";
const char* SUCCESS_HTTP_RESP = "200 ok";
const char* LINE_BREAK = "\r\n";
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
	{"AT+CHTTPSSTART"},
	{""}
};

char response[RESPONSE_SIZE] = {};
char http_response[RESPONSE_SIZE] = {};
bool _http_busy = false;


void sim_proccess_input(const char input_chr)
{
	static uint8_t line_break_counter = 0;

	if (strlen(response) >= RESPONSE_SIZE - 1) {
		_shift_response();
	}

	response[strlen(response)] = input_chr;

	_update_line_counter(&line_break_counter);

	if (strstr(response, SUCCESS_CMD_RESP)) {
		LOG_DEBUG(SIM_TAG, "%s\r\n", response);
		sim_state.state |= CMD_SUCCESS;
		sim_state.state &= ~WAIT_MODULE;
		_clear_response();
	}

	if (strstr(response, SUCCESS_HTTP_ACT)) {
		LOG_DEBUG(SIM_TAG, "%s\r\n", response);
		sim_state.state |= HTTP_ACT_SUCCESS;
		sim_state.state &= ~WAIT_HTTP;
		_clear_response();
	}

	if (strstr(response, SUCCESS_HTTP_RESP)) {
		LOG_DEBUG(SIM_TAG, "%s\r\n", response);
		sim_state.state |= HTTP_SUCCESS;
		sim_state.state &= ~WAIT_HTTP;
		_clear_response();
	}

	if (_if_http_success() && line_break_counter > LINE_BREAK_COUNT) {
		_clear_response();
		HAL_UART_Receive(&SIM_MODULE_UART, http_response, RESPONSE_SIZE, UART_TIMEOUT);
		line_break_counter = 0;
		sim_state.state = LOAD_SUCCESS;
		LOG_DEBUG(SIM_TAG, " HTTP \r\n%s\r\n", http_response);
	}
}

void sim_module_begin() {
	memset(&sim_state, 0, sizeof(sim_state));
	Util_TimerStart(&sim_state.uart_timer, UART_WAIT);
	Util_TimerStart(&sim_state.restart_timer, RESTART_WAIT);
	_start_module();
	_clear_response();
}

void sim_module_proccess()
{
	if (_if_load_success()) {
		return;
	}

	if (!_if_has_error() && !Util_TimerPending(&sim_state.uart_timer)) {
		sim_state.state = ERROR_STATE;
		_reset_module();
		Util_TimerStart(&sim_state.restart_timer, RESTART_WAIT);
	}

	if (_if_has_error() && !Util_TimerPending(&sim_state.restart_timer)) {
		sim_state.state = RESET_STATE;
		_start_module();
		Util_TimerStart(&sim_state.uart_timer, UART_WAIT);
	}

	if (_if_wait_module()) {
		return;
	}

	if (!_if_has_error()) {
		_send_config_command();
	}
}

void connect_to_server()
{
	char http_act[HTTP_ACT_SIZE] = {};
	snprintf(http_act, sizeof(http_act), "AT+CHTTPACT=\"%s\",%s\r\n", module_settings.server_url, module_settings.server_port);
	_send_AT_command(http_act);
}

void send_http(const char* data)
{
	if (is_server_available()) {
		return;
	}
	if (!strlen(data)) {
		return;
	}
	_http_busy = true;
	_send_AT_command(data);
}

bool is_module_ready()
{
	return _if_module_ready() && _if_gprs_ready();
}

bool is_server_available()
{
	return !_if_has_error() && _if_http_act_success();
}

bool is_http_success()
{
	return !_if_has_error() && _if_http_success();
}

bool is_http_busy()
{
	return _http_busy;
}

char* get_response()
{
	_http_busy = false;
	return http_response;
}

void _send_config_command()
{
	static uint8_t conf_pos = 0;
	static uint8_t gprs_pos = 0;

	if (_if_has_error() || _if_reset_state()) {
		LOG_DEBUG(SIM_TAG, " error response: %s\r\n", response);
		conf_pos = 0;
		gprs_pos = 0;
		sim_state.state &= ~ERROR_STATE;
		_clear_response();
	}

	if (!strlen(sim_config_list[conf_pos]) && !strlen(gprs_config_list[gprs_pos])) {
		sim_state.state = LOAD_SUCCESS;
		return;
	}

	if (!strlen(sim_config_list[conf_pos])) {
		sim_state.state |= MODULE_READY;
	}

	if (!_if_module_ready()) {
		LOG_DEBUG(SIM_TAG, " %s\r\n", sim_config_list[conf_pos]);
		_send_AT_command(sim_config_list[conf_pos]);
		sim_state.state |= WAIT_MODULE;
		conf_pos++;
	}

	if (_if_module_ready()) {
		LOG_DEBUG(SIM_TAG, " %s\r\n", gprs_config_list[gprs_pos]);
		_send_AT_command(gprs_config_list[gprs_pos]);
		sim_state.state |= WAIT_MODULE;
		gprs_pos++;
	}

	Util_TimerStart(&sim_state.uart_timer, UART_WAIT);
}

void _send_AT_command(const char* command)
{
	uint8_t cmd_size = strlen(command) + strlen(LINE_BREAK) + 1;
	char *at_cmd = (char*)malloc((cmd_size + 1) * sizeof(char));
	snprintf(at_cmd, cmd_size, " %s%s", command, LINE_BREAK);
	HAL_UART_Transmit(&SIM_MODULE_UART, at_cmd, strlen(at_cmd), UART_TIMEOUT);
}

void _update_line_counter(uint8_t *line_break_counter)
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

void _stop_http()
{
    _send_AT_command("AT+CHTTPSSTOP");
}

void _reset_module()
{
	_http_busy = false;
	HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_RESET);
}

void _start_module()
{
	HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_SET);
}

bool _if_reset_state()
{
	return !sim_state.state;
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

bool _if_http_act_success()
{
	return sim_state.state & HTTP_ACT_SUCCESS;
}

bool _if_http_success()
{
	return sim_state.state & HTTP_SUCCESS;
}

bool _if_has_error()
{
	return sim_state.state & ERROR_STATE;
}

bool _if_load_success()
{
	return _if_module_ready() && _if_gprs_ready() && _if_http_success();
}
