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


void _send_uart_response(const char* message);
void _clear_command();
void _split_command_string();
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


char char_buffer = 0,
     command_buffer[CHAR_COMMAND_SIZE] = {},
     command[2][CHAR_COMMAND_SIZE] = {{}, {}};
const char *COMMAND_TAG = "UARTCMD";

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (validate_command()) {
		execute_command();
	}
	HAL_UART_Receive_IT(&COMMAND_UART, &char_buffer, sizeof(char));
}

void command_manager_begin()
{
	_clear_command();
	HAL_UART_Receive_IT(&COMMAND_UART, &char_buffer, sizeof(char));
}

bool validate_command()
{
	if (strlen(command_buffer) == CHAR_COMMAND_SIZE) {
		memset(command_buffer, 0, sizeof(command_buffer));
		LOG_DEBUG(COMMAND_TAG, "Invalid UART command\r\n");
		return false;
	}
	if (char_buffer != '\n') {
		strcpy(command_buffer + strlen(command_buffer), &char_buffer);
		return false;
	}

	_trim_command();
	_split_command_string();

	if (!strlen(command[0])) {
		_clear_command();
		return false;
	}
	if (!strlen(command[0]) || strlen(command[0]) > CHAR_SETIINGS_SIZE) {
		_clear_command();
		return false;
	}
	return true;
}

/**
 * TODO: описание для команд
 */
void execute_command()
{
	if (strcmp("setid", command[0]) == 0) {
		module_settings.id = (uint32_t)atoi(command[1]);
		_show_id();
	} else if (strcmp("setsleepingtime", command[0]) == 0) {
		module_settings.sleep_time = atoi(command[1]);
		_show_sleeping_time();
	} else if (strcmp("setserverurl", command[0]) == 0) {
		strcpy(module_settings.server_url, command[1]);
		_show_server_url();
	} else if (strcmp("setserverport", command[0]) == 0) {
		strcpy(module_settings.server_port, command[1]);
		_show_server_port();
	} else if (strcmp("setlitersmin", command[0]) == 0) {
		module_settings.tank_liters_min = atoi(command[1]);
		_show_liters_min();
	} else if (strcmp("setlitersmax", command[0]) == 0) {
		module_settings.tank_liters_max = atoi(command[1]);
		_show_liters_max();
	} else if (strcmp("setcurrentlitersmin", command[0]) == 0) {
//		module_settings.tank_ADC_min = get_liquid_ADC_value();
		_show_liters_ADC_min();
	} else if (strcmp("setcurrentlitersmax", command[0]) == 0) {
//		module_settings.tank_ADC_max = get_liquid_ADC_value();
		_show_liters_ADC_max();
	} else if (strcmp("setmillilitersperday", command[0]) == 0) {
		module_settings.milliliters_per_day = atoi(command[1]);
		_show_liters_per_month();
	} else if (strcmp("setpumpspeed", command[0]) == 0) {
		module_settings.pump_speed = (uint32_t)atoi(command[1]);
		_show_pump_speed();
	} else if (strcmp("eraseflash", command[0]) == 0) {
		// TODO: remove file
//		erase_flash();
		_send_uart_response("FLASH erased\r\n");
	} else {
		_send_uart_response("Invalid UART command\r\n");
		_clear_command();
		return;
	}

	if (!settings_save()) {
		_send_uart_response("ERROR SAVE SETTINS\r\n");
	}

	_clear_command();
}

void _send_uart_response(const char* message)
{
	HAL_UART_Transmit(&COMMAND_UART, message, strlen(message), DEFAULT_UART_DELAY);
}

void _clear_command()
{
	memset(command_buffer, 0, sizeof(command_buffer));
	memset(command[0], 0, sizeof(command[0]));
	memset(command[1], 0, sizeof(command[1]));
}

void _split_command_string()
{
	printf("SPLIT:\r\n");
	char* token = strtok(command_buffer, " ");
	if (token == NULL) {
		return;
	}
	strcpy(command[0], token);
	_send_uart_response(token);

	token = strtok(NULL, " ");
	if (token == NULL) {
		strcpy(command[0], command_buffer);
		_send_uart_response(command[0]);
		return;
	}
	strcpy(command[1], token);
	_send_uart_response(token);
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
	sprintf(response, "Device ID: %d\r\n", module_settings.id);
	_send_uart_response(response);
}

void _show_server_url()
{
	char response[UART_RESPONSE_SIZE] = {};
	sprintf(response, "URL: %s\r\n", module_settings.server_url);
	_send_uart_response(response);
}

void _show_server_port()
{
	char response[UART_RESPONSE_SIZE] = {};
	sprintf(response, "PORT: %s\r\n", module_settings.server_port);
	_send_uart_response(response);
}

void _show_liters_max()
{
	char response[UART_RESPONSE_SIZE] = {};
	sprintf(response, "Liliters max: %d l\r\n", module_settings.tank_liters_max);
	_send_uart_response(response);
}

void _show_liters_min()
{
	char response[UART_RESPONSE_SIZE] = {};
	sprintf(response, "Liliters min: %d l\r\n", module_settings.tank_liters_min);
	_send_uart_response(response);
}

void _show_liters_ADC_max()
{
	char response[UART_RESPONSE_SIZE] = {};
	sprintf(response, "ADC max: %d\r\n", module_settings.tank_ADC_max);
	_send_uart_response(response);
}

void _show_liters_ADC_min()
{
	char response[UART_RESPONSE_SIZE] = {};
	sprintf(response, "ADC min: %d\r\n", module_settings.tank_ADC_min);
	_send_uart_response(response);
}

void _show_liters_per_month()
{
	char response[UART_RESPONSE_SIZE] = {};
	sprintf(response, "Milliliters per day: %d ml/d\r\n", module_settings.milliliters_per_day);
	_send_uart_response(response);
}

void _show_pump_speed()
{
	char response[UART_RESPONSE_SIZE] = {};
	sprintf(response, "Pump speed: %d ml/h\r\n", module_settings.pump_speed);
	_send_uart_response(response);
}

void _show_sleeping_time()
{
	char response[UART_RESPONSE_SIZE] = {};
	sprintf(response, "Sleep time: %d sec\r\n", module_settings.sleep_time / MILLIS_IN_SECOND);
	_send_uart_response(response);
}
