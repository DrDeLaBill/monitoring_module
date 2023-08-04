/*
 * sim_module.c
 *
 *  Created on: Sep 4, 2022
 *      Author: DrDeLaBill
 */

#include "sim_module.h"

#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>

#include "settings_manager.h"
#include "utils.h"
#include "liquid_sensor.h"
#include "logger.h"
#include "main.h"


#define LINE_BREAK_COUNT 2
#define UART_TIMEOUT     100
#define END_OF_STRING    0x1a
#define MAX_ERRORS       5
#define CNFG_WAIT        10000
#define HTTP_ACT_WAIT    15000
#define RESTART_WAIT     5000
#define HTTP_ACT_SIZE    90


void _set_active_state(void (*cmd_state) (void));
void _execute_state(const char* cmd, uint16_t delay);
void _send_AT_command(const char* cmd);
void _check_response(const char* needed_resp);
void _check_response_headers();
void _check_response_timer();
void _validate_response(const char* needed_resp);
void _do_error(uint8_t attempts);
bool _if_network_wait();

void _cmd_AT_state();
void _cmd_ATE0_state();
void _cmd_CPIN_state();
void _cmd_CTZU_state();
void _cmd_CCLK_state();
void _cmd_CGDCONT_state();
void _cmd_CGSOCKCONT_state();
void _cmd_CSOCKSETPN_state();
void _cmd_CHTTPSSTART_state();
void _cmd_CHTTPACT_state();
void _cmd_send_state();
void _cmd_CHTTPSSTOP_state();

void _check_200_ok_itr();
void _check_content_length_itr();
void _check_double_break_itr();
void _wait_data_itr();
void _wait_itr();

void _clear_response();
void _reset_module();
void _start_module();
void _sim_set_error_response_state();

enum {
	WAIT = 0,
	READY,
	SIM_SUCCESS,
	SIM_ERROR
};

struct _sim_state {
	void (*active_cmd_state) (void);
	void (*check_http_header_itr) (void);
	dio_timer_t start_timer;
	dio_timer_t delay_timer;
	dio_timer_t restart_timer;
	uint8_t state;
	uint8_t error_count;
	char session_server_url[CHAR_SETIINGS_SIZE];
	char session_server_port[CHAR_SETIINGS_SIZE];
} sim_state;

const char* SIM_TAG = "SIM";

const char* SUCCESS_CMD_RESP  = "ok";
const char* SUCCESS_HTTP_ACT  = "+chttpact: request";
const char* HTTP_ACT_COUNT    = "+chttpact: ";
const char* SUCCESS_HTTP_RQST = "content-length: ";
const char* SUCCESS_HTTP_RESP = "200 ok";
const char* LINE_BREAK        = "\r\n";
const char* DOUBLE_LINE_BREAK = "\r\n\r\n";
const char* SIM_ERR_RESPONSE  = "\r\nerror\r\n";

char sim_response[RESPONSE_SIZE] = {};
uint16_t response_counter = 0;
uint16_t response_data_count = 0;

void sim_module_begin() {
	memset(&sim_state, 0, sizeof(sim_state));
	strncpy(sim_state.session_server_url, module_settings.server_url, sizeof(sim_state.session_server_url));
	strncpy(sim_state.session_server_port, module_settings.server_port, sizeof(sim_state.session_server_port));
	util_timer_start(&sim_state.start_timer, CNFG_WAIT);
	_set_active_state(&_cmd_AT_state);
	_start_module();
	_clear_response();
}

void sim_module_proccess()
{
	if (util_is_timer_wait(&sim_state.start_timer)) {
		return;
	}

	if (sim_state.active_cmd_state != NULL) {
		sim_state.active_cmd_state();
	}
}

void sim_proccess_input(const char input_chr)
{
	sim_response[response_counter++] = tolower(input_chr);
	if (_if_network_wait()) {
		sim_state.check_http_header_itr();
	}
	if (response_counter >= sizeof(sim_response) - 1) {
		_clear_response();
	}
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
		"POST /api/log/ep HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: monitor\r\n"
		"Connection: close\r\n"
		"Accept-Charset: utf-8, us-ascii\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: %u\r\n"
		"\r\n"
		"%s"
		"%c",
		sim_state.session_server_url,
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

bool _if_network_wait()
{
	return sim_state.active_cmd_state == &_cmd_send_state && sim_state.state == WAIT;
}

char* get_response()
{
	_set_active_state(&_cmd_CHTTPSSTOP_state);
	return sim_response;
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
	util_timer_start(&sim_state.delay_timer, delay);
}

void _send_AT_command(const char* cmd)
{
	HAL_UART_Transmit(&SIM_MODULE_UART, (uint8_t*)cmd, strlen(cmd), UART_TIMEOUT);
	HAL_UART_Transmit(&SIM_MODULE_UART, (uint8_t*)LINE_BREAK, strlen(LINE_BREAK), UART_TIMEOUT);
#if SIM_MODULE_DEBUG
	LOG_DEBUG(SIM_TAG, "send - %s\r\n", cmd);
#endif
}

void _check_response(const char* needed_resp)
{
	_check_response_timer();

	_validate_response(needed_resp);
}

void _check_200_ok_itr() {
	char* ptr = strnstr(sim_response, SUCCESS_HTTP_RESP, response_counter);
	if (!ptr) {
		return;
	}
	memset(sim_response, 0, response_counter);
	sim_state.check_http_header_itr = &_check_content_length_itr;
	response_counter = 0;
}

void _check_content_length_itr() {
	char* ptr = strnstr(sim_response, SUCCESS_HTTP_RQST, response_counter);
	if (!ptr) {
		return;
	}
	ptr += strlen(SUCCESS_HTTP_RQST);

	if (!strnstr(ptr, LINE_BREAK, strlen(ptr))) {
		return;
	}
	response_data_count = atoi(ptr);

	memset(sim_response, 0, response_counter);
	sim_state.check_http_header_itr = &_check_double_break_itr;
	response_counter = 0;
}

void _check_double_break_itr() {
	char* ptr = strnstr(sim_response, DOUBLE_LINE_BREAK, response_counter);
	if (!ptr) {
		return;
	}
	memset(sim_response, 0, ptr - sim_response);
	strncpy(sim_response, ptr, strlen(ptr));
	memset(ptr, 0, strlen(ptr));
	response_counter = strlen(ptr);
	sim_state.check_http_header_itr = &_wait_data_itr;
}

void _wait_data_itr() {
	if (response_counter >= response_data_count) {
		sim_state.state = SIM_SUCCESS;
#if SIM_MODULE_DEBUG
		LOG_DEBUG(SIM_TAG, "HTTP response -\n%s\n", sim_response);
#endif
		sim_state.check_http_header_itr = &_wait_itr;
	}
}

void _wait_itr() {}

void _check_response_timer()
{
	if (util_is_timer_wait(&sim_state.delay_timer)) {
		return;
	}

	_sim_set_error_response_state();

	if (sim_state.active_cmd_state != _cmd_send_state) {
		return;
	}

	if (sim_state.error_count < MAX_ERRORS) {
		return;
	}

	if (strncmp(module_settings.server_url, default_server_url, sizeof(module_settings.server_url))) {
		strncpy(sim_state.session_server_url, default_server_url, sizeof(sim_state.session_server_url));
		strncpy(sim_state.session_server_port, default_server_port, sizeof(sim_state.session_server_port));
	} else {
		strncpy(sim_state.session_server_url, module_settings.server_url, sizeof(sim_state.session_server_url));
		strncpy(sim_state.session_server_port, module_settings.server_port, sizeof(sim_state.session_server_port));
	}
}

void _validate_response(const char* needed_resp)
{
	if (strnstr(sim_response, needed_resp, sizeof(sim_response))) {
#if SIM_MODULE_DEBUG
		LOG_DEBUG(SIM_TAG, "success - %s\n", sim_response);
#endif
		sim_state.state = SIM_SUCCESS;
		sim_state.error_count = 0;
	} else if (strnstr(sim_response, SIM_ERR_RESPONSE, sizeof(sim_response))) {
		_sim_set_error_response_state();
	}
}

void _sim_set_error_response_state()
{
#if SIM_MODULE_DEBUG
	LOG_DEBUG(SIM_TAG, "error - %s\n", strlen(sim_response) ? sim_response : "empty answer");
#endif
	sim_state.state = SIM_ERROR;
	sim_state.error_count++;
}

void _do_error(uint8_t attempts)
{
	if (util_is_timer_wait(&sim_state.restart_timer)) {
		_set_active_state(&_cmd_AT_state);
		sim_state.state = SIM_ERROR;
		util_timer_start(&sim_state.start_timer, RESTART_WAIT);
		return;
	}

	if (sim_state.error_count < attempts) {
#if SIM_MODULE_DEBUG
		LOG_DEBUG(SIM_TAG, "retry command, attempt number %d\n", sim_state.error_count + 1);
#endif
		sim_state.state = READY;
		_clear_response();
		_start_module();
		return;
	}

	if (sim_state.error_count >= attempts) {
#if SIM_MODULE_DEBUG
		LOG_DEBUG(SIM_TAG, "too many errors\n");
#endif
		_reset_module();
		util_timer_start(&sim_state.restart_timer, RESTART_WAIT);
		return;
	}
}

void _clear_response()
{
	memset(sim_response, 0, sizeof(sim_response));
	sim_state.check_http_header_itr = &_check_200_ok_itr;
	response_counter = 0;
	response_data_count = 0;
}

void _reset_module()
{
	if (util_is_timer_wait(&sim_state.restart_timer)) {
		return;
	}
#if SIM_MODULE_DEBUG
	LOG_DEBUG(SIM_TAG, "sim module RESTART\n");
#endif
	HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_RESET);
}

void _start_module()
{
	if (util_is_timer_wait(&sim_state.restart_timer)) {
		return;
	}
	util_timer_start(&sim_state.start_timer, CNFG_WAIT);
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
		_set_active_state(&_cmd_CTZU_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_CTZU_state()
{
	if (sim_state.state == READY) {
		_execute_state("AT+CTZU=1", CNFG_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response(SUCCESS_CMD_RESP);
	}
	if (sim_state.state == SIM_SUCCESS) {
		_set_active_state(&_cmd_CCLK_state);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _cmd_CCLK_state()
{
	if (sim_state.state == READY) {
		_execute_state("AT+CCLK?", CNFG_WAIT);
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
		snprintf(http_act, sizeof(http_act), "AT+CHTTPACT=\"%s\",%s\r\n", sim_state.session_server_url, sim_state.session_server_port);
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
		util_timer_start(&sim_state.delay_timer, HTTP_ACT_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response_timer();
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
