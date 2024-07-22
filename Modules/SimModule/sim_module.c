/*
 * sim_module.c
 *
 *  Created on: Sep 4, 2022
 *      Author: DrDeLaBill
 */

#include "sim_module.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>

#include "glog.h"
#include "main.h"
#include "pump.h"
#include "gutils.h"
#include "settings.h"
#include "liquid_sensor.h"


#define LINE_BREAK_COUNT 2
#define UART_TIMEOUT     100
#define END_OF_STRING    0x1a
#define MAX_ERRORS       5
#define CNFG_WAIT        10000
#define HTTP_ACT_WAIT    20000
#define RESTART_WAIT     5000
#define HTTP_ACT_SIZE    90
#define LOG_SIZE         140


void _execute_state(const char* cmd, uint16_t delay);
void _send_AT_command(const char* cmd);
void _check_response(const char* needed_resp);
void _check_response_headers();
void _check_response_timer();
void _validate_response(const char* needed_resp);
void _do_error(uint8_t attempts);
bool _if_network_wait();
bool _if_setup_ready();

void _cmd_setup_proccess();
void _cmd_http_proccess();

void _http_HTTPPARA_fsm();
void _http_HTTPDATA_fsm();
void _http_send_fsm();
void _http_HTTPACTION_fsm();
void _http_HTTPHEAD_fsm();
void _http_HTTPREAD_fsm();
void _http_wait_fsm();
void _http_HTTPTERM_fsm();

void _clear_http();
void _reset_http();
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
    void (*http_fsm) (void);
    util_old_timer_t start_timer;
    util_old_timer_t delay_timer;
    util_old_timer_t restart_timer;
    uint8_t state;
    uint8_t error_count;
    char session_server_url[CHAR_SETIINGS_SIZE];
    uint8_t sim_command_idx;

    char sim_request[LOG_SIZE];
    char sim_response[RESPONSE_SIZE];
    uint16_t response_counter;
    uint32_t content_length;
} sim_state;


extern settings_t settings;


const char* SIM_TAG = "SIM";

const char* SUCCESS_CMD_RESP  = "ok";
const char* SUCCESS_HTTP_ACT  = "+chttpact: request";
const char* HTTP_ACT_COUNT    = "+chttpact: ";
const char* CONTENT_LENGTH    = "content-length: ";
const char* SUCCESS_HTTP_RESP = "200 ok";
const char* LINE_BREAK        = "\r\n";
const char* DOUBLE_LINE_BREAK = "\r\n\r\n";
const char* SIM_ERR_RESPONSE  = "\r\nerror\r\n";


typedef struct _sim_command_t {
    char request [30];
    char response[20];
} sim_command_t;


sim_command_t commands[] = {
	{"AT",          "ok"},
	{"ATE0",        "ok"},
	{"AT+CGMR",     "ok"},
	{"AT+CSQ",      "ok"},
//	{"AT+CPIN?",    "+cpin: ready"},
	{"AT+CGREG?",   "+cgreg: 0,1"},
	{"AT+CPSI?",    "ok"},
	{"AT+CGDCONT?", "ok"},
	{"AT+HTTPINIT", "ok"}
};

void sim_module_begin() {
    memset(&sim_state, 0, sizeof(sim_state));
    strncpy(sim_state.session_server_url, settings.url, sizeof(sim_state.session_server_url));
    util_old_timer_start(&sim_state.start_timer, CNFG_WAIT);
    _start_module();
    _reset_http();
}

void sim_module_proccess()
{
    if (util_old_timer_wait(&sim_state.start_timer)) {
        return;
    }

    _cmd_setup_proccess();
}

void sim_proccess_input(const char input_chr)
{
    sim_state.sim_response[sim_state.response_counter++] = (uint8_t)tolower(input_chr);
    if (sim_state.response_counter >= sizeof(sim_state.sim_response) - 1) {
        _clear_http();
    }
}

void send_http_post(const char* data)
{
    if (sim_state.state == WAIT) {
        return;
    }
    memset(sim_state.sim_request, 0, sizeof(sim_state.sim_request));
    snprintf(
		sim_state.sim_request,
        sizeof(sim_state.sim_request),
        "%s"
        "%c",
        data,
        END_OF_STRING
    );
    sim_state.state       = READY;
    sim_state.error_count = 0;
}

char* get_response()
{
    sim_state.http_fsm        = &_http_HTTPTERM_fsm;
    sim_state.state           = READY;
    sim_state.error_count     = 0;
    return sim_state.sim_response;
}

bool if_network_ready()
{
	return sim_state.http_fsm == _http_HTTPDATA_fsm;
}

bool has_http_response()
{
    return sim_state.http_fsm == _http_wait_fsm;
}

bool _if_network_wait()
{
    return _if_setup_ready() && !has_http_response();
}

bool _if_setup_ready()
{
    return sim_state.sim_command_idx == __arr_len(commands);
}

void _execute_state(const char* cmd, uint16_t delay)
{
    _clear_http();
    _send_AT_command(cmd);
    sim_state.state = WAIT;
    util_old_timer_start(&sim_state.delay_timer, delay);
}

void _send_AT_command(const char* cmd)
{
    HAL_UART_Transmit(&SIM_MODULE_UART, (uint8_t*)cmd, (uint16_t)strlen(cmd), UART_TIMEOUT);
    HAL_UART_Transmit(&SIM_MODULE_UART, (uint8_t*)LINE_BREAK, (uint16_t)strlen(LINE_BREAK), UART_TIMEOUT);
#if SIM_MODULE_DEBUG
    printTagLog(SIM_TAG, "send - %s\r\n", cmd);
#endif
}

void _check_response(const char* needed_resp)
{
    _check_response_timer();

    _validate_response(needed_resp);
}

void _check_response_timer()
{
    if (util_old_timer_wait(&sim_state.delay_timer)) {
        return;
    }

    _sim_set_error_response_state();

    if (!_if_network_wait()) {
        return;
    }

    if (sim_state.error_count < MAX_ERRORS) {
        return;
    }

    if (strncmp(settings.url, defaultUrl, sizeof(settings.url))) {
        strncpy(sim_state.session_server_url, defaultUrl, sizeof(sim_state.session_server_url));
        printTagLog(SIM_TAG, "Change server url to: %s", defaultUrl);
    } else {
        strncpy(sim_state.session_server_url, settings.url, sizeof(sim_state.session_server_url));
        printTagLog(SIM_TAG, "Change server url to: %s", settings.url);
    }
}

void _validate_response(const char* needed_resp)
{
    if (strnstr(sim_state.sim_response, needed_resp, sizeof(sim_state.sim_response))) {
#if SIM_MODULE_DEBUG
        printTagLog(SIM_TAG, "success - [%s]\n", sim_state.sim_response);
#endif
        sim_state.state       = SIM_SUCCESS;
        sim_state.error_count = 0;
    } else if (strnstr(sim_state.sim_response, SIM_ERR_RESPONSE, sizeof(sim_state.sim_response))) {
        _sim_set_error_response_state();
    }
}

void _sim_set_error_response_state()
{
#if SIM_MODULE_DEBUG
    printTagLog(SIM_TAG, "error - [%s]\n", strlen(sim_state.sim_response) ? sim_state.sim_response : "empty answer");
#endif
    sim_state.state = SIM_ERROR;
    sim_state.error_count++;
}

void _do_error(uint8_t attempts)
{
    if (util_old_timer_wait(&sim_state.restart_timer)) {
        sim_state.sim_command_idx = 0;
        sim_state.state           = READY;
        sim_state.error_count     = 0;
        sim_state.http_fsm        = _http_HTTPTERM_fsm;
        sim_state.state = SIM_ERROR;
        util_old_timer_start(&sim_state.start_timer, RESTART_WAIT);
        return;
    }

    if (sim_state.error_count < attempts) {
#if SIM_MODULE_DEBUG
        printTagLog(SIM_TAG, "retry command, attempt number %d\n", sim_state.error_count + 1);
#endif
        sim_state.state = READY;
        _reset_http();
        _start_module();
        return;
    }

    if (sim_state.error_count >= attempts) {
#if SIM_MODULE_DEBUG
        printTagLog(SIM_TAG, "too many errors\n");
#endif
        _reset_module();
        util_old_timer_start(&sim_state.restart_timer, RESTART_WAIT);
        return;
    }
}

void _clear_http()
{
    memset(sim_state.sim_response, 0, sizeof(sim_state.sim_response));
    sim_state.response_counter = 0;
}

void _reset_http()
{
	memset(sim_state.sim_request, 0, sizeof(sim_state.sim_request));
    memset(sim_state.sim_response, 0, sizeof(sim_state.sim_response));
    sim_state.http_fsm = &_http_HTTPPARA_fsm;
    sim_state.state = READY;
    sim_state.content_length = 0;
    sim_state.response_counter = 0;
}

void _reset_module()
{
    if (util_old_timer_wait(&sim_state.restart_timer)) {
        return;
    }
#if SIM_MODULE_DEBUG
    printTagLog(SIM_TAG, "sim module RESTART\n");
#endif
    HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_RESET);
}

void _start_module()
{
    if (util_old_timer_wait(&sim_state.restart_timer)) {
        return;
    }
    util_old_timer_start(&sim_state.start_timer, CNFG_WAIT);
    HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_SET);
}

void _cmd_setup_proccess() {
    if (_if_setup_ready()) {
        _cmd_http_proccess();
        return;
    }

    if (sim_state.state == READY) {
        _execute_state(commands[sim_state.sim_command_idx].request, CNFG_WAIT);
    }
    if (sim_state.state == WAIT) {
        _check_response(commands[sim_state.sim_command_idx].response);
    }
    if (sim_state.state == SIM_SUCCESS) {
        sim_state.sim_command_idx++;
        sim_state.state       = READY;
        sim_state.error_count = 0;
        _reset_http();
    }
    if (sim_state.state == SIM_ERROR) {
        _do_error(MAX_ERRORS);
    }
}

void _cmd_http_proccess()
{
	if (!sim_state.http_fsm) {
		_reset_http();
	}

	sim_state.http_fsm();
}

void _http_HTTPPARA_fsm()
{
	if (sim_state.state == READY) {
		char httppara[HTTP_ACT_SIZE] = { 0 };
		snprintf(httppara, sizeof(httppara), "AT+HTTPPARA=\"URL\",\"http://%s/api/log/ep\"", sim_state.session_server_url);
		_execute_state(httppara, HTTP_ACT_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response("ok");
	}
	if (sim_state.state == SIM_SUCCESS) {
		sim_state.http_fsm    = &_http_HTTPDATA_fsm;
        sim_state.state       = READY;
        sim_state.error_count = 0;
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _http_HTTPDATA_fsm()
{
	if (!strlen(sim_state.sim_request)) {
		return;
	}

	if (sim_state.state == READY) {
		char httpdata[HTTP_ACT_SIZE] = { 0 };
		snprintf(httpdata, sizeof(httpdata), "AT+HTTPDATA=%d,%d", strlen(sim_state.sim_request), 1000);
		_execute_state(httpdata, HTTP_ACT_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response("download");
	}
	if (sim_state.state == SIM_SUCCESS) {
		sim_state.http_fsm    = &_http_send_fsm;
        sim_state.state       = READY;
        sim_state.error_count = 0;
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _http_send_fsm()
{
	if (sim_state.state == READY) {
		_execute_state(sim_state.sim_request, HTTP_ACT_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response("ok");
	}
	if (sim_state.state == SIM_SUCCESS) {
		sim_state.http_fsm    = &_http_HTTPACTION_fsm;
        sim_state.state       = READY;
        sim_state.error_count = 0;
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _http_HTTPACTION_fsm()
{
	if (sim_state.state == READY) {
		_execute_state("AT+HTTPACTION=1", HTTP_ACT_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response("+httpaction: 1,200");
	}
	if (sim_state.state == SIM_SUCCESS) {
		sim_state.http_fsm    = &_http_HTTPHEAD_fsm;
        sim_state.state       = READY;
        sim_state.error_count = 0;
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _http_HTTPHEAD_fsm()
{
	if (sim_state.state == READY) {
		sim_state.content_length = 0;
		_execute_state("AT+HTTPHEAD", HTTP_ACT_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response("\r\nok\r\n");
	}
	if (sim_state.state == SIM_SUCCESS) {
		sim_state.http_fsm    = &_http_HTTPREAD_fsm;
        sim_state.state       = READY;
        sim_state.error_count = 0;
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _http_HTTPREAD_fsm()
{
	if (sim_state.state == READY) {
		sim_state.content_length = (uint32_t)atoi(
			strnstr(
				sim_state.sim_response,
				CONTENT_LENGTH,
				sizeof(sim_state.sim_response)
			) + strlen(CONTENT_LENGTH)
		);
		char request[HTTP_ACT_SIZE] = { 0 };
		snprintf(request, sizeof(request), "AT+HTTPREAD=0,%lu", sim_state.content_length);
		_execute_state(request, HTTP_ACT_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response("+httpread: 0");
	}
	if (sim_state.state == SIM_SUCCESS) {
		sim_state.http_fsm    = &_http_wait_fsm;
        sim_state.state       = READY;
        sim_state.error_count = 0;
        util_old_timer_start(&sim_state.delay_timer, HTTP_ACT_WAIT);
	}
	if (sim_state.state == SIM_ERROR) {
		_do_error(MAX_ERRORS);
	}
}

void _http_wait_fsm()
{
	if (util_old_timer_wait(&sim_state.delay_timer)) {
		return;
	}
	sim_state.http_fsm    = &_http_HTTPTERM_fsm;
    sim_state.state       = READY;
    sim_state.error_count = 0;
}

void _http_HTTPTERM_fsm()
{
	if (sim_state.state == READY) {
		_execute_state("AT+HTTPTERM", HTTP_ACT_WAIT);
	}
	if (sim_state.state == WAIT) {
		_check_response("ok");
	}
	if (sim_state.state == SIM_SUCCESS || sim_state.state == SIM_ERROR) {
		sim_state.http_fsm    = &_http_HTTPPARA_fsm;
        sim_state.state       = READY;
        sim_state.error_count = 0;
        _reset_http();
        sim_state.sim_command_idx = 0;
	}
}
