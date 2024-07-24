/*
 * sim_module.c
 *
 *  Created on: Sep 4, 2022
 *      Author: DrDeLaBill
 */

#include "../sim/sim_module.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>

#include "glog.h"
#include "main.h"
#include "pump.h"
#include "fsm_gc.h"
#include "gutils.h"
#include "settings.h"

#include "liquid_sensor.h"


#define A7670_ENABLE     (0)
#define SIM868E_ENABLE   (1)

#define SIM_MAX_ERRORS   (5)
#define SIM_DELAY_MS     (10000)
#define SIM_HTTP_MS      (20000)
#define SIM_HTTP_SIZE    (90)


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
    char request [50];
    char response[20];
} sim_command_t;


typedef struct _sim_state_t {
	bool     done;
	unsigned counter;

	char     url[CHAR_SETIINGS_SIZE];

	char     request[SIM_LOG_SIZE];
	char     response[RESPONSE_SIZE];
	unsigned resp_cnt;
	unsigned resp_len;

	util_old_timer_t timer;

	unsigned errors;
} sim_state_t;


sim_state_t sim_state = {0};


const sim_command_t start_cmds[] = {
	{"AT",          "ok"},
	{"ATE0",        "ok"},
	{"AT+CGMR",     "ok"},
	{"AT+CSQ",      "ok"},
#if A7670_ENABLE
	{"AT+CPIN?",    "+cpin: ready"},
	{"AT+CGREG?",   "+cgreg: 0,1"},
	{"AT+CPSI?",    "ok"},
	{"AT+CGDCONT?", "ok"}
#elif SIM868E_ENABLE
	{"AT+COPS?",     "ok"},
	{"AT+SAPBR=2,1", "ok"},
	{"AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "ok"},
	{"AT+SAPBR=3,1,\"APN\",\"internet\"", "ok"},
	{"AT+SAPBR=1,1", "ok"}
#else
#   error "Please select your modem"
#endif
};

const sim_command_t sim868_cmds[] = {
	{"AT+CREG?",    "ok"},
	{"AT+IFC=1,1",  "ok"},
	{"AT+CLIP=1",   "ok"},
	{"AT+CLTS=1",   "ok"},
	{"AT+CSCLK=0",  "ok"},
	{"AT+CMGHEX=1", "ok"},
	{"AT+CNTP",     "ok"},
	{"AT+HTTPINIT", "ok"}
};

const sim_command_t a7670_cmds[] = {
	{"AT+CPIN?",    "+cpin: ready"},
	{"AT+CGREG?",   "+cgreg: 0,1"},
	{"AT+CPSI?",    "ok"},
	{"AT+CGDCONT?", "ok"},
	{"AT+HTTPINIT", "ok"}
};


void _sim_send_cmd(const char* cmd);
void _sim_clear_response();
bool _sim_validate(const char* target);

void _sim_init_s(void);
void _sim_start_s(void);
void _sim_start_iterate_s(void);
void _sim_check_sim_s(void);
void _sim_check_sim_wait_s(void);
void _sim_868e_start_s(void);
void _sim_868e_iterate_s(void);
void _sim_a7670e_start_s(void);
void _sim_a7670e_iterate_s(void);
void _sim_init_http_s(void);
void _sim_start_http_s(void);
void _sim_send_http_s(void);
void _sim_send_post_s(void);
void _sim_wait_post_s(void);
void _sim_read_data_s(void);
void _sim_wait_data_s(void);
void _sim_wait_user_s(void);
void _sim_close_http_s(void);
void _sim_change_url_s(void);
void _sim_count_error_s(void);
void _sim_reset_s(void);
void _sim_error_s(void);


FSM_GC_CREATE(sim_fsm)

FSM_GC_CREATE_EVENT(sim_success_e)
FSM_GC_CREATE_EVENT(sim_end_e)
FSM_GC_CREATE_EVENT(sim_a7670e_e)
FSM_GC_CREATE_EVENT(sim_868e_e)
FSM_GC_CREATE_EVENT(sim_timeout_e)
FSM_GC_CREATE_EVENT(sim_error_e)

FSM_GC_CREATE_STATE(sim_init_s,           _sim_init_s)
FSM_GC_CREATE_STATE(sim_start_s,          _sim_start_s)
FSM_GC_CREATE_STATE(sim_start_iterate_s,  _sim_start_iterate_s)
FSM_GC_CREATE_STATE(sim_check_sim_s,      _sim_check_sim_s)
FSM_GC_CREATE_STATE(sim_check_sim_wait_s, _sim_check_sim_wait_s)
FSM_GC_CREATE_STATE(sim_868e_start_s,     _sim_868e_start_s)
FSM_GC_CREATE_STATE(sim_868e_iterate_s,   _sim_868e_iterate_s)
FSM_GC_CREATE_STATE(sim_a7670e_start_s,   _sim_a7670e_start_s)
FSM_GC_CREATE_STATE(sim_a7670e_iterate_s, _sim_a7670e_iterate_s)
FSM_GC_CREATE_STATE(sim_init_http_s,      _sim_init_http_s)
FSM_GC_CREATE_STATE(sim_start_http_s,     _sim_start_http_s)
FSM_GC_CREATE_STATE(sim_send_http_s,      _sim_send_http_s)
FSM_GC_CREATE_STATE(sim_send_post_s,      _sim_send_post_s)
FSM_GC_CREATE_STATE(sim_wait_post_s,      _sim_wait_post_s)
FSM_GC_CREATE_STATE(sim_read_data_s,      _sim_read_data_s)
FSM_GC_CREATE_STATE(sim_wait_data_s,      _sim_wait_data_s)
FSM_GC_CREATE_STATE(sim_wait_user_s,      _sim_wait_user_s)
FSM_GC_CREATE_STATE(sim_close_http_s,     _sim_close_http_s)
FSM_GC_CREATE_STATE(sim_change_url_s,     _sim_change_url_s)
FSM_GC_CREATE_STATE(sim_count_error_s,    _sim_count_error_s)
FSM_GC_CREATE_STATE(sim_reset_s,          _sim_reset_s)
FSM_GC_CREATE_STATE(sim_error_s,          _sim_error_s)

FSM_GC_CREATE_TABLE(
	sim_fsm_table,
	{&sim_init_s,           &sim_success_e,  &sim_reset_s},

	{&sim_start_s,          &sim_success_e,  &sim_start_iterate_s},

	{&sim_start_iterate_s,  &sim_success_e,  &sim_start_s},
	{&sim_start_iterate_s,  &sim_timeout_e,  &sim_count_error_s},
	{&sim_start_iterate_s,  &sim_end_e,      &sim_init_http_s},

//	{&sim_check_sim_s,      &sim_success_e,  &sim_check_sim_wait_s},
//
//	{&sim_check_sim_wait_s, &sim_a7670e_e,   &sim_a7670e_start_s},
//	{&sim_check_sim_wait_s, &sim_868e_e,     &sim_868e_start_s},
//	{&sim_check_sim_wait_s, &sim_timeout_e,  &sim_count_error_s},
//
//	{&sim_868e_start_s,     &sim_success_e,  &sim_868e_iterate_s},
//
//	{&sim_868e_iterate_s,   &sim_success_e,  &sim_868e_start_s},
//	{&sim_868e_iterate_s,   &sim_timeout_e,  &sim_count_error_s},
//	{&sim_868e_iterate_s,   &sim_end_e,      &sim_start_http_s},
//
//	{&sim_a7670e_start_s,   &sim_success_e,  &sim_a7670e_iterate_s},
//
//	{&sim_a7670e_iterate_s, &sim_success_e,  &sim_a7670e_start_s},
//	{&sim_a7670e_iterate_s, &sim_timeout_e,  &sim_count_error_s},
//	{&sim_a7670e_iterate_s, &sim_end_e,      &sim_start_http_s},

	{&sim_init_http_s,      &sim_success_e,  &sim_start_http_s},
	{&sim_init_http_s,      &sim_timeout_e,  &sim_count_error_s},

	{&sim_start_http_s,     &sim_success_e,  &sim_send_http_s},
	{&sim_start_http_s,     &sim_timeout_e,  &sim_change_url_s},

	{&sim_send_http_s,      &sim_success_e,  &sim_send_post_s},
	{&sim_send_http_s,      &sim_timeout_e,  &sim_change_url_s},

	{&sim_send_post_s,      &sim_success_e,  &sim_wait_post_s},
	{&sim_send_post_s,      &sim_timeout_e,  &sim_change_url_s},

	{&sim_wait_post_s,      &sim_success_e,  &sim_read_data_s},
	{&sim_wait_post_s,      &sim_timeout_e,  &sim_change_url_s},

	{&sim_read_data_s,      &sim_success_e,  &sim_wait_data_s},
	{&sim_read_data_s,      &sim_timeout_e,  &sim_change_url_s},

	{&sim_wait_data_s,      &sim_success_e,  &sim_wait_user_s},
	{&sim_wait_data_s,      &sim_timeout_e,  &sim_change_url_s},

	{&sim_wait_user_s,      &sim_success_e,  &sim_close_http_s},

	{&sim_close_http_s,     &sim_success_e,  &sim_init_http_s},
	{&sim_close_http_s,     &sim_timeout_e,  &sim_count_error_s},

	{&sim_change_url_s,     &sim_success_e,  &sim_count_error_s},

	{&sim_count_error_s,    &sim_success_e,  &sim_start_s},
	{&sim_count_error_s,    &sim_error_e,    &sim_error_s},

	{&sim_error_s,          &sim_success_e,  &sim_reset_s},

	{&sim_reset_s,          &sim_success_e,  &sim_start_s}
)


void sim_begin() {
	fsm_gc_init(&sim_fsm, sim_fsm_table, __arr_len(sim_fsm_table));
}

void sim_proccess()
{
	fsm_gc_proccess(&sim_fsm);
}

void sim_proccess_input(const char input_chr)
{
    sim_state.response[sim_state.resp_cnt++] = (uint8_t)tolower(input_chr);
    if (sim_state.resp_cnt >= sizeof(sim_state.response) - 1) {
		_sim_clear_response();
    }
}

void send_sim_http_post(const char* data)
{
    if (!fsm_gc_is_state(&sim_fsm, &sim_send_http_s) || sim_state.done) {
        return;
    }
    memset(sim_state.request, 0, sizeof(sim_state.request));
    snprintf(
		sim_state.request,
        sizeof(sim_state.request),
        "%s"
        "%c",
        data,
        END_OF_STRING
    );
    sim_state.done = true;
}

char* get_response()
{
	sim_state.done = true;
    return sim_state.response;
}

bool if_network_ready()
{
	return fsm_gc_is_state(&sim_fsm, &sim_send_http_s);
}

bool has_http_response()
{
    return fsm_gc_is_state(&sim_fsm, &sim_wait_user_s);
}

void _sim_send_cmd(const char* cmd)
{
    HAL_UART_Transmit(&SIM_MODULE_UART, (uint8_t*)cmd, (uint16_t)strlen(cmd), GENERAL_TIMEOUT_MS);
    HAL_UART_Transmit(&SIM_MODULE_UART, (uint8_t*)LINE_BREAK, (uint16_t)strlen(LINE_BREAK), GENERAL_TIMEOUT_MS);
#if SIM_MODULE_DEBUG
    printTagLog(SIM_TAG, "send - %s\r\n", cmd);
#endif
}

bool _sim_validate(const char* target)
{
    if (strnstr(sim_state.response, target, sizeof(sim_state.response))) {
#if SIM_MODULE_DEBUG
        printTagLog(SIM_TAG, "success - [%s]\n", sim_state.response);
#endif
        return true;
    }
    return false;
}

void _sim_clear_response()
{
    memset(sim_state.response, 0, sizeof(sim_state.response));
    sim_state.resp_cnt = 0;
}


void _sim_init_s(void)
{
	memset(&sim_state, 0, sizeof(sim_state));
	strncpy(sim_state.url, settings.url, sizeof(sim_state.url));

	sim_state.counter = 0;
	util_old_timer_start(&sim_state.timer, 1500);

	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}

void _sim_start_s(void)
{
	memset(sim_state.response, 0, sizeof(sim_state.response));
	_sim_send_cmd(start_cmds[sim_state.counter].request);
	util_old_timer_start(&sim_state.timer, SIM_DELAY_MS);
	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}

void _sim_start_iterate_s(void)
{
	if (_sim_validate(start_cmds[sim_state.counter].response)) {
		sim_state.counter++;
		sim_state.errors = 0;
		_sim_clear_response();
		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}

	if (sim_state.counter >= __arr_len(start_cmds)) {
		sim_state.counter = 0;
		sim_state.errors  = 0;

		fsm_gc_clear(&sim_fsm);
		_sim_clear_response();
		fsm_gc_push_event(&sim_fsm, &sim_end_e);
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_check_sim_s(void)
{
	memset(sim_state.response, 0, sizeof(sim_state.response));
	_sim_send_cmd("ATI");

	util_old_timer_start(&sim_state.timer, 20000);

	fsm_gc_clear(&sim_fsm);
	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}

void _sim_check_sim_wait_s(void)
{
	if (_sim_validate("a7670")) {
		sim_state.counter = 0;
		_sim_clear_response();
		fsm_gc_push_event(&sim_fsm, &sim_a7670e_e);
		return;
	}

	if (_sim_validate("sim868")) {
		sim_state.counter = 0;
		_sim_clear_response();
		fsm_gc_push_event(&sim_fsm, &sim_868e_e);
		return;
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_868e_start_s(void)
{
	memset(sim_state.response, 0, sizeof(sim_state.response));
	_sim_send_cmd(sim868_cmds[sim_state.counter].request);
	util_old_timer_start(&sim_state.timer, SIM_DELAY_MS);
	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}

void _sim_868e_iterate_s(void)
{
	if (_sim_validate(sim868_cmds[sim_state.counter].response)) {
		sim_state.counter++;
		_sim_clear_response();
		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}

	if (sim_state.counter >= __arr_len(sim868_cmds)) {
		sim_state.counter = 0;
		_sim_clear_response();
		fsm_gc_clear(&sim_fsm);
		fsm_gc_push_event(&sim_fsm, &sim_end_e);
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}


void _sim_a7670e_start_s(void)
{
	memset(sim_state.response, 0, sizeof(sim_state.response));
	_sim_send_cmd(a7670_cmds[sim_state.counter].request);
	util_old_timer_start(&sim_state.timer, SIM_DELAY_MS);
	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}

void _sim_a7670e_iterate_s(void)
{
	if (_sim_validate(a7670_cmds[sim_state.counter].response)) {
		sim_state.counter++;
		_sim_clear_response();
		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}

	if (sim_state.counter >= __arr_len(a7670_cmds)) {
		sim_state.counter = 0;
		_sim_clear_response();
		fsm_gc_clear(&sim_fsm);
		fsm_gc_push_event(&sim_fsm, &sim_end_e);
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_init_http_s(void)
{
	if (!sim_state.counter) {
		sim_state.counter++;
		_sim_clear_response();

		_sim_send_cmd("AT+HTTPINIT");
		util_old_timer_start(&sim_state.timer, 5000);
	}

	if (_sim_validate("ok")) {
		sim_state.counter = 0;
		_sim_clear_response();

		fsm_gc_clear(&sim_fsm);
		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_start_http_s(void)
{
	if (!sim_state.counter) {
		sim_state.counter++;
		_sim_clear_response();

		char httppara[SIM_HTTP_SIZE] = { 0 };
		snprintf(httppara, sizeof(httppara), "AT+HTTPPARA=\"URL\",\"http://%s/api/log/ep\"", sim_state.url);

		_sim_send_cmd(httppara);
		util_old_timer_start(&sim_state.timer, 5000);
	}

	if (_sim_validate("ok")) {
		sim_state.done = false;
		sim_state.counter = 0;
		_sim_clear_response();
		fsm_gc_clear(&sim_fsm);
		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_send_http_s(void)
{
	if (!sim_state.done) {
		return;
	}

	if (!sim_state.counter) {
		sim_state.counter++;
		_sim_clear_response();

		char httpdata[SIM_HTTP_SIZE] = { 0 };
		snprintf(httpdata, sizeof(httpdata), "AT+HTTPDATA=%d,%d", strlen(sim_state.request), 1000);
		memset(sim_state.response, 0, sizeof(sim_state.response));
		_sim_send_cmd(httpdata);
		util_old_timer_start(&sim_state.timer, SIM_HTTP_MS);
	}

	if (_sim_validate("download")) {
		_sim_clear_response();

		fsm_gc_clear(&sim_fsm);

		_sim_send_cmd(sim_state.request);
		util_old_timer_start(&sim_state.timer, SIM_HTTP_MS);

		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_send_post_s(void)
{
	if (_sim_validate("ok")) {
		_sim_clear_response();
		sim_state.done = false;

		fsm_gc_clear(&sim_fsm);

		_sim_send_cmd("AT+HTTPACTION=1");
		util_old_timer_start(&sim_state.timer, SIM_HTTP_MS);

		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_wait_post_s(void)
{
	const char* httpaction = "+httpaction: 1,200,";

	if (!sim_state.done && _sim_validate(httpaction)) {
		sim_state.done = true;
	} else if (sim_state.done) {
		char* ptr = strnstr(
			sim_state.response,
			httpaction,
			sizeof(sim_state.response)
		) + strlen(httpaction);
		char format[32] = {0};
		snprintf(format, sizeof(format) - 1, "%s%u\r\n", httpaction, (unsigned)atoi(ptr));
		if (strnstr(sim_state.response, format, sizeof(sim_state.response))) {
			sim_state.resp_len = (unsigned)atoi(ptr);
			sim_state.done = false;

			fsm_gc_clear(&sim_fsm);

			_sim_send_cmd("AT+HTTPHEAD");
			util_old_timer_start(&sim_state.timer, SIM_HTTP_MS);

			fsm_gc_push_event(&sim_fsm, &sim_success_e);
		}
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_read_data_s(void)
{
	const char* contentlength = "content-length: ";

	if (!sim_state.done && _sim_validate(contentlength)) {
		sim_state.done = true;
	} else if (sim_state.done) {
		char* ptr = strnstr(
			sim_state.response,
			contentlength,
			sizeof(sim_state.response)
		) + strlen(contentlength);
		char format[32] = {0};
		snprintf(format, sizeof(format) - 1, "%s%u\r\n", contentlength, (unsigned)atoi(ptr));
		if (strnstr(sim_state.response, format, sizeof(sim_state.response))) {
			sim_state.resp_len = (unsigned)atoi(ptr);
			sim_state.done = false;
			_sim_clear_response();

			fsm_gc_clear(&sim_fsm);

			char request[SIM_HTTP_SIZE] = { 0 };
			snprintf(request, sizeof(request), "AT+HTTPREAD=0,%u", sim_state.resp_len);
			_sim_send_cmd(request);
			util_old_timer_start(&sim_state.timer, SIM_HTTP_MS);

			fsm_gc_push_event(&sim_fsm, &sim_success_e);
		}
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_wait_data_s(void)
{
	if (_sim_validate("\r\n\r\nok\r\n")) {
		fsm_gc_clear(&sim_fsm);

		sim_state.done = false;
		util_old_timer_start(&sim_state.timer, SIM_HTTP_MS);

		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}

void _sim_wait_user_s(void)
{
	if (!sim_state.done && util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	_sim_clear_response();

	_sim_send_cmd("AT+HTTPTERM");
	util_old_timer_start(&sim_state.timer, SIM_HTTP_MS);

	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}

void _sim_close_http_s(void)
{
	if (_sim_validate("ok")) {
		_sim_clear_response();

		fsm_gc_clear(&sim_fsm);
		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	fsm_gc_push_event(&sim_fsm, &sim_timeout_e);
}

void _sim_change_url_s(void)
{
    if (strncmp(settings.url, defaultUrl, sizeof(settings.url))) {
        strncpy(sim_state.url, defaultUrl, sizeof(sim_state.url));
#if SIM_MODULE_DEBUG
        printTagLog(SIM_TAG, "Change server url to: %s", sim_state.url);
#endif
    } else {
        strncpy(sim_state.url, settings.url, sizeof(sim_state.url));
#if SIM_MODULE_DEBUG
        printTagLog(SIM_TAG, "Change server url to: %s", settings.url);
#endif
    }

	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}

void _sim_count_error_s(void)
{
#if SIM_MODULE_DEBUG
    printTagLog(SIM_TAG, "error - [%s]\n", strlen(sim_state.response) ? sim_state.response : "empty answer");
#endif
	_sim_clear_response();

	sim_state.errors++;

	if (sim_state.errors > SIM_MAX_ERRORS) {
		fsm_gc_push_event(&sim_fsm, &sim_error_e);
	} else {
		fsm_gc_push_event(&sim_fsm, &sim_success_e);
	}
}

void _sim_error_s(void)
{
#if SIM_MODULE_DEBUG
	printTagLog(SIM_TAG, "too many errors\n");
#endif

	memset(&sim_state, 0, sizeof(sim_state));
	strncpy(sim_state.url, settings.url, sizeof(sim_state.url));

	util_old_timer_start(&sim_state.timer, 1500);

	fsm_gc_clear(&sim_fsm);
	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}

void _sim_reset_s(void)
{
	HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_RESET);

	if (util_old_timer_wait(&sim_state.timer)) {
		return;
	}

	HAL_GPIO_WritePin(SIM_MODULE_RESET_PORT, SIM_MODULE_RESET_PIN, GPIO_PIN_SET);
	fsm_gc_push_event(&sim_fsm, &sim_success_e);
}
