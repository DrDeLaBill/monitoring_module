/*
 * sim_module.c
 *
 *  Created on: Sep 4, 2022
 *      Author: georg
 */

#include "sim_module.h"

#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>

#include "settings.h"
#include "utils.h"
#include "ina3221_sensor.h"
#include "liquid_sensor.h"
#include "logger.h"


#define LINE_BREAK_COUNT 2
#define UART_TIMEOUT     100
#define END_OF_STRING    0x1a
#define MAX_ERRORS       5
#define CNFG_WAIT        10000
#define HTTP_ACT_WAIT    15000
#define RESTART_WAIT     5000
#define HTTP_ACT_SIZE    60


void _set_active_state(void (*cmd_state) (void));
void _execute_state(const char* cmd, uint16_t delay);
void _send_AT_command(const char* cmd);
void _check_response(const char* needed_resp);
void _check_http_response();
void _check_response_timer();
void _validate_response(char* needed_resp);
void _do_error(uint8_t attempts);

void _cmd_AT_state();
void _cmd_ATE0_state();
void _cmd_CPIN_state();
void _cmd_CGDCONT_state();
void _cmd_CGSOCKCONT_state();
void _cmd_CSOCKSETPN_state();
void _cmd_CHTTPSSTART_state();
void _cmd_CHTTPACT_state();
void _cmd_send_state();
void _cmd_CHTTPSSTOP_state();

void _clear_response();
void _reset_module();
void _start_module();
void _shift_response();

enum {
	WAIT = 0,
	READY,
	SIM_SUCCESS,
	SIM_ERROR
};

struct _sim_state {
	void (*active_cmd_state) (void);
	dio_timer_t start_timer;
	dio_timer_t delay_timer;
	dio_timer_t restart_timer;
	uint8_t state;
	uint8_t error_count;
} sim_state;

const char* SIM_TAG = "SIM";

const char* SUCCESS_CMD_RESP  = "ok";
const char* SUCCESS_HTTP_ACT  = "+chttpact: request";
const char* SUCCESS_HTTP_RQST = "content-length: ";
const char* SUCCESS_HTTP_RESP = "200 ok";
const char* LINE_BREAK        = "\r\n";
const char* DOUBLE_LINE_BREAK = "\r\n\r\n";

char response[RESPONSE_SIZE] = {};


void sim_module_begin() {
	memset(&sim_state, 0, sizeof(sim_state));
	Util_TimerStart(&sim_state.start_timer, CNFG_WAIT);
	_set_active_state(&_cmd_AT_state);
	_start_module();
	_clear_response();
}

void sim_module_proccess()
{
	if (Util_TimerPending(&sim_state.start_timer)) {
		return;
	}

	if (sim_state.active_cmd_state != NULL) {
		sim_state.active_cmd_state();
	}
}

void sim_proccess_input(const char input_chr)
{
	if (strlen(response) >= sizeof(response) - 1) {
		_shift_response();
	}

	response[strlen(response)] = tolower(input_chr);
}

void send_http_post(const char* data)
{
	if (sim_state.state == WAIT) {
		return;
	}
	char request[LOG_SIZE] = {};
	snprintf(
		request,
		sizeof(request),
		"POST /api/v1/send HTTP/1.1\r\n"
		"Host: %s:%s\r\n"
		"User-Agent: zhelezyaka\r\n"
		"Connection: close\r\n"
		"Accept-Charset: utf-8, us-ascii\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: %u\r\n"
		"\r\n"
		"%s"
		"%c",
		module_settings.server_url,
		module_settings.server_port,
		strlen(data),
		data,
		END_OF_STRING
	);
	_execute_state(request, HTTP_ACT_WAIT);
}

bool has_http_response()
{
	return sim_state.active_cmd_state == _cmd_send_state && sim_state.state == SIM_SUCCESS;
}

bool if_network_ready()
{
	return sim_state.active_cmd_state == &_cmd_send_state && sim_state.state == READY;
}

char* get_response()
{
	_set_active_state(&_cmd_CHTTPSSTOP_state);
	return response;
}

void _set_active_state(void (*cmd_state) (void)) {
	sim_state.active_cmd_state = cmd_state;
	sim_state.state = READY;
	sim_state.error_count = 0;
}

void _execute_state(const char* cmd, uint16_t delay)
{
	_clear_response();
	_send_AT_command(cmd);
	sim_state.state = WAIT;
	Util_TimerStart(&sim_state.delay_timer, delay);
}

void _send_AT_command(const char* cmd)
{
	HAL_UART_Transmit(&SIM_MODULE_UART, (uint8_t*)cmd, strlen(cmd), UART_TIMEOUT);
	HAL_UART_Transmit(&SIM_MODULE_UART, (uint8_t*)LINE_BREAK, strlen(LINE_BREAK), UART_TIMEOUT);
	LOG_DEBUG(SIM_TAG, " send - %s\r\n", cmd);
}

void _check_response(const char* needed_resp)
{
	_check_response_timer();

	_validate_response(needed_resp);
}

void _check_http_response()
{
	_check_response_timer();

//	char *ptr = strnstr(response, SUCCESS_HTTP_RQST, strlen(response));
//	if (!ptr) {
//		return;
//	}
//
//	ptr += strlen(SUCCESS_HTTP_RQST);
//
//	if (!strlen(ptr)) {
//		return;
//	}
//
//	uint16_t size = atoi(ptr);
//	ptr = strnstr(ptr, DOUBLE_LINE_BREAK, strlen(ptr));
//	if (!ptr) {
//		return;
//	}
//	ptr += strlen(DOUBLE_LINE_BREAK);
//
//	if (strlen(ptr) >= size) {
//		return;
//	}
//
//	_validate_response(SUCCESS_HTTP_RESP);
}

void _check_response_timer()
{
	if (!Util_TimerPending(&sim_state.delay_timer)) {
		LOG_DEBUG(SIM_TAG, " error - %s\r\n", response);
		sim_state.state = SIM_ERROR;
		sim_state.error_count++;
	}
}

void _validate_response(char* needed_resp)
{
	if (strnstr(response, needed_resp, sizeof(response))) {
		LOG_DEBUG(SIM_TAG, " success - %s\r\n", response);
		sim_state.state = SIM_SUCCESS;
		sim_state.error_count = 0;
	}
}

void _do_error(uint8_t attempts)
{
	if (Util_TimerPending(&sim_state.restart_timer)) {
		_set_active_state(&_cmd_AT_state);
		sim_state.state = SIM_ERROR;
		Util_TimerStart(&sim_state.start_timer, RESTART_WAIT);
		return;
	}

	if (sim_state.error_count < attempts) {
		LOG_DEBUG(SIM_TAG, " retry command, attempt number %d\r\n", sim_state.error_count + 1);
		sim_state.state = READY;
		_clear_response();
		_start_module();
		return;
	}

	if (sim_state.error_count >= attempts) {
		LOG_DEBUG(SIM_TAG, " too many errors\r\n");
		_reset_module();
		Util_TimerStart(&sim_state.restart_timer, RESTART_WAIT);
		return;
	}
}

void _clear_response()
{
	memset(response, 0, sizeof(response));
}

void _shift_response()
{
	LOG_DEBUG(SIM_TAG, " shift response - %s\r\n", response);
	strncpy(response, response + sizeof(response) / 2, sizeof(response));
	memset(response + sizeof(response) / 2, 0, sizeof(response) / 2);
}

void _reset_module()
{
	if (Util_TimerPending(&sim_state.restart_timer)) {
		return;
	}
	LOG_DEBUG(SIM_TAG, " sim module RESTART\r\n");
	HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_RESET);
}

void _start_module()
{
	if (Util_TimerPending(&sim_state.restart_timer)) {
		return;
	}
	Util_TimerStart(&sim_state.start_timer, CNFG_WAIT);
	HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_SET);
}


void _cmd_AT_state()
{
	if (sim_state.state == READY) {
		_execute_state("AT", CNFG_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_CMD_RESP);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_ATE0_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_ATE0_state()
{
	if (sim_state.state == READY) {
		_execute_state("ATE0", CNFG_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_CMD_RESP);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_CPIN_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_CPIN_state()
{
	if (sim_state.state == READY) {
		_execute_state("AT+CPIN?", CNFG_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_CMD_RESP);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_CGDCONT_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_CGDCONT_state()
{
	if (sim_state.state == READY) {
		_execute_state("AT+CGDCONT=1,\"IP\",\"internet\",\"0.0.0.0\"", CNFG_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_CMD_RESP);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_CGSOCKCONT_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_CGSOCKCONT_state()
{
	if (sim_state.state == READY) {
		_execute_state("AT+CGSOCKCONT=1,\"IP\",\"internet\"", CNFG_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_CMD_RESP);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_CSOCKSETPN_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_CSOCKSETPN_state()
{
	if (sim_state.state == READY) {
		_execute_state("AT+CSOCKSETPN=1", CNFG_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_CMD_RESP);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_CHTTPSSTART_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_CHTTPSSTART_state()
{
	if (sim_state.state == READY) {
		_execute_state("AT+CHTTPSSTART", CNFG_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_CMD_RESP);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_CHTTPACT_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_CHTTPACT_state()
{
	if (sim_state.state == READY) {
		char http_act[HTTP_ACT_SIZE] = {};
		snprintf(http_act, sizeof(http_act), "AT+CHTTPACT=\"%s\",%s\r\n", module_settings.server_url, module_settings.server_port);
		_execute_state(http_act, HTTP_ACT_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_HTTP_ACT);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_send_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_send_state()
{
	if (sim_state.state == READY) {
		Util_TimerStart(&sim_state.delay_timer, HTTP_ACT_WAIT);
		_set_active_state(&_cmd_send_state);
	}
	if (sim_state.state == WAIT) {
		_check_http_response();
	}
	if (sim_state.state == SIM_ERROR) {
		_set_active_state(&_cmd_CHTTPSSTOP_state);
	}
}

void _cmd_CHTTPSSTOP_state()
{
	if (sim_state.state == READY) {
		_execute_state("AT+CHTTPSSTOP", CNFG_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_CMD_RESP);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_CHTTPSSTART_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}
