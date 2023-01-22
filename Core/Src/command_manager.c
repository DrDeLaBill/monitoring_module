/*
 * uart_command.c
 *
 *  Created on: Sep 5, 2022
 *      Author: georg
 */

#include <command_manager.h>

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "settings.h"
#include "settings_manager.h"
#include "liquid_sensor.h"
#include "utils.h"
#include "ds1307_for_stm32_hal.h"


bool _validate_command();
void _execute_command();
void _send_uart_response(const char* message);
void _clear_command();
void _trim_command();
void _clear_command();
void _show_id();
void _show_server_url();
void _show_server_port();
void _show_liters_max();
void _show_liters_min();
void _show_liters_ADC_max();
void _show_liters_ADC_min();
void _show_liters_per_month();
void _show_pump_speed();
void _show_sleeping_time();
void _show_server_log_id();
void _show_settings();
void _show_time();


const char *COMMAND_TAG = "UARTCMD";
char command_buffer[CHAR_COMMAND_SIZE] = {};

void cmd_proccess_input(const char input_chr)
{
	if (strlen(command_buffer) >= CHAR_COMMAND_SIZE) {
		_clear_command();
	}
	command_buffer[strlen(command_buffer)] = input_chr;
}

void command_manager_begin()
{
	_clear_command();
	HAL_UART_Receive_IT(&COMMAND_UART, (uint8_t*) command_buffer, sizeof(char));
}

void command_manager_proccess()
{
	if (_validate_command()) {
		_execute_command();
	}
}

bool _validate_command()
{
	if (strlen(command_buffer) == 0) {
		return false;
	}

	if (strlen(command_buffer) >= CHAR_COMMAND_SIZE) {
		memset(command_buffer, 0, sizeof(command_buffer));
		LOG_DEBUG(COMMAND_TAG, "Invalid UART command\n");
		return false;
	}

	if (command_buffer[strlen(command_buffer)-1] == '\n') {
		command_buffer[strlen(command_buffer)-1] = 0;
		return true;
	}

	return false;
}

/**
 * TODO: описание для команд
 */
void _execute_command()
{
    char command[2][CHAR_COMMAND_SIZE] = {{}, {}};

	_trim_command();

	char* token = strtok(command_buffer, " ");
	if (token == NULL) {
		_clear_command();
	}
	strncpy(command[0], token, CHAR_COMMAND_SIZE);

	token = strtok(NULL, " ");
	if (token == NULL) {
		strncpy(command[0], command_buffer, CHAR_COMMAND_SIZE);
	}
	strncpy(command[1], token, CHAR_COMMAND_SIZE);

	if (!strlen(command[0])) {
		_clear_command();
	}

	if (strlen(command[0]) > CHAR_SETIINGS_SIZE) {
		_clear_command();
	}

	if (strncmp("settings", command[0], CHAR_COMMAND_SIZE) == 0) {
		_show_settings();
		goto do_end;
	} else if (strncmp("time", command[0], CHAR_COMMAND_SIZE) == 0) {
		_show_time();
		goto do_end;
	} else if (strncmp("setid", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.id = (uint32_t)atoi(command[1]);
		_show_id();
	} else if (strncmp("setsleep", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.sleep_time = atoi(command[1]) * 1000;
		_show_sleeping_time();
	} else if (strncmp("seturl", command[0], CHAR_COMMAND_SIZE) == 0) {
		strncpy(module_settings.server_url, command[1], sizeof(module_settings.server_url));
		_show_server_url();
	} else if (strncmp("setport", command[0], CHAR_COMMAND_SIZE) == 0) {
		strncpy(module_settings.server_port, command[1], sizeof(module_settings.server_port));
		_show_server_port();
	} else if (strncmp("setlitersmin", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_liters_min = atoi(command[1]);
		_show_liters_min();
	} else if (strncmp("setlitersmax", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_liters_max = atoi(command[1]);
		_show_liters_max();
	} else if (strncmp("saveadcmmin", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_ADC_min = get_liquid_level();
		_show_liters_ADC_min();
	} else if (strncmp("saveadcax", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_ADC_max = get_liquid_level();
		_show_liters_ADC_max();
	} else if (strncmp("settarget", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.milliliters_per_day = atoi(command[1]);
		_show_liters_per_month();
	} else if (strncmp("setpumpspeed", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.pump_speed = (uint32_t)atoi(command[1]);
		_show_pump_speed();
	} else if (strncmp("setlogid", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.server_log_id = (uint32_t)atoi(command[1]);
		_show_server_log_id();
	} else if (strncmp("default", command[0], CHAR_COMMAND_SIZE) == 0) {
		settings_reset();
	} else {
		_send_uart_response("Invalid UART command\n");
		_clear_command();
		goto do_end;
	}

	settings_save();
	_show_settings();

do_end:
	_clear_command();
}

void _send_uart_response(const char* message)
{
	HAL_UART_Transmit(&COMMAND_UART, (uint8_t*) message, strlen(message), DEFAULT_UART_DELAY);
}

void _clear_command()
{
	memset(command_buffer, 0, sizeof(command_buffer));
}

void _trim_command()
{
	for (uint8_t i = 0; i < strlen(command_buffer); i++) {
		if (command_buffer[i] == '\r' || command_buffer[0] == '\n') {
			command_buffer[i] = ' ';
		}
	}
}

void _show_id()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "Device ID: %lu\n", module_settings.id);
	_send_uart_response(response);
}

void _show_server_url()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "URL: %s\n", module_settings.server_url);
	_send_uart_response(response);
}

void _show_server_port()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "PORT: %s\n", module_settings.server_port);
	_send_uart_response(response);
}

void _show_liters_max()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "Liliters max: %hu l\n", module_settings.tank_liters_max);
	_send_uart_response(response);
}

void _show_liters_min()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "Liliters min: %hu l\n", module_settings.tank_liters_min);
	_send_uart_response(response);
}

void _show_liters_ADC_max()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "ADC max: %hu\n", module_settings.tank_ADC_max);
	_send_uart_response(response);
}

void _show_liters_ADC_min()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "ADC min: %hu\n", module_settings.tank_ADC_min);
	_send_uart_response(response);
}

void _show_liters_per_month()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "Milliliters per day: %lu ml/d\n", module_settings.milliliters_per_day);
	_send_uart_response(response);
}

void _show_pump_speed()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "Pump speed: %lu ml/h\n", module_settings.pump_speed);
	_send_uart_response(response);
}

void _show_sleeping_time()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "Sleep time: %lu sec\n", module_settings.sleep_time / MILLIS_IN_SECOND);
	_send_uart_response(response);
}

void _show_server_log_id()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, UART_RESPONSE_SIZE, "Log ID: %lu\n", module_settings.server_log_id);
	_send_uart_response(response);
}

void _show_time()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(
		response,
		sizeof(response),
		"\nTime: %lu-%02u-%02uT%02u:%02u:%02u\n",
		DS1307_GetYear(),
		DS1307_GetMonth(),
		DS1307_GetDate(),
		DS1307_GetHour(),
		DS1307_GetMinute(),
		DS1307_GetSecond()
	);
	_send_uart_response(response);
}

void _show_settings()
{
	_show_id();
	_show_server_url();
	_show_server_port();
	_show_liters_max();
	_show_liters_min();
	_show_liters_ADC_max();
	_show_liters_ADC_min();
	_show_liters_per_month();
	_show_pump_speed();
	_show_sleeping_time();
	_show_server_log_id();
}
