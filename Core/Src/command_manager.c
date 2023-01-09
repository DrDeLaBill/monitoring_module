/*
 * uart_command.c
 *
 *  Created on: Sep 5, 2022
 *      Author: georg
 */

#include <command_manager.h>

#include "stm32f1xx_hal.h"
#include <stdbool.h>

#include "settings.h"
#include "settings_manager.h"
#include "utils.h"


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


const char *COMMAND_TAG = "UARTCMD";
char command_buffer[CHAR_COMMAND_SIZE] = {};

void cmd_proccess_input(const char input_chr)
{
	if (strlen(command_buffer) >= CHAR_COMMAND_SIZE) {
		_clear_command();
	}
	command_buffer[strlen(command_buffer)] = input_chr;
	if (_validate_command()) {
		_execute_command();
	}
}

void command_manager_begin()
{
	_clear_command();
	HAL_UART_Receive_IT(&COMMAND_UART, command_buffer, sizeof(char));
}

bool _validate_command()
{
	if (strlen(command_buffer) == 0) {
		return false;
	}

	if (strlen(command_buffer) >= CHAR_COMMAND_SIZE) {
		memset(command_buffer, 0, sizeof(command_buffer));
		LOG_DEBUG(COMMAND_TAG, "Invalid UART command\r\n");
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

	if (strncmp("setid", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.id = (uint32_t)atoi(command[1]);
		_show_id();
	} else if (strncmp("setsleep", command[0], CHAR_COMMAND_SIZE) == 0) {
		module_settings.sleep_time = atoi(command[1]);
		_show_sleeping_time();
	} else if (strncmp("seturl", command[0], CHAR_COMMAND_SIZE) == 0) {
		strncpy(module_settings.server_url, command[1], CHAR_COMMAND_SIZE);
		_show_server_url();
	} else if (strncmp("setport", command[0], CHAR_COMMAND_SIZE) == 0) {
		strncpy(module_settings.server_port, command[1], CHAR_COMMAND_SIZE);
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
	} else if (strncmp("setpumpspeed", command[0]) == 0, CHAR_COMMAND_SIZE) {
		module_settings.pump_speed = (uint32_t)atoi(command[1]);
		_show_pump_speed();
	} else if (strncmp("default", command[0], CHAR_COMMAND_SIZE) == 0) {
		settings_reset();
	} else {
		_send_uart_response("Invalid UART command\r\n");
		_clear_command();
		return;
	}

	settings_save();
	_clear_command();
}

void _send_uart_response(const char* message)
{
	HAL_UART_Transmit(&COMMAND_UART, message, strlen(message), DEFAULT_UART_DELAY);
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
	snprintf(response, "Device ID: %d\r\n", module_settings.id, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}

void _show_server_url()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, "URL: %s\r\n", module_settings.server_url, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}

void _show_server_port()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, "PORT: %s\r\n", module_settings.server_port, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}

void _show_liters_max()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, "Liliters max: %d l\r\n", module_settings.tank_liters_max, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}

void _show_liters_min()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, "Liliters min: %d l\r\n", module_settings.tank_liters_min, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}

void _show_liters_ADC_max()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, "ADC max: %d\r\n", module_settings.tank_ADC_max, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}

void _show_liters_ADC_min()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, "ADC min: %d\r\n", module_settings.tank_ADC_min, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}

void _show_liters_per_month()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, "Milliliters per day: %d ml/d\r\n", module_settings.milliliters_per_day, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}

void _show_pump_speed()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, "Pump speed: %d ml/h\r\n", module_settings.pump_speed, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}

void _show_sleeping_time()
{
	char response[UART_RESPONSE_SIZE] = {};
	snprintf(response, "Sleep time: %d sec\r\n", module_settings.sleep_time / MILLIS_IN_SECOND, UART_RESPONSE_SIZE);
	_send_uart_response(response);
}
