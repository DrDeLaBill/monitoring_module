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
#include "logger.h"
#include "settings_manager.h"
#include "record_manager.h"
#include "liquid_sensor.h"
#include "utils.h"
#include "ds1307_for_stm32_hal.h"
#include "pump.h"


bool _validate_command();
void _execute_command();
void _clear_command();
void _show_status();
void _show_error();


const char *COMMAND_TAG = "CMND";
char command_buffer[CHAR_COMMAND_SIZE] = {};


void cmd_proccess_input(const char input_chr)
{
	if (strlen(command_buffer) >= CHAR_COMMAND_SIZE) {
		_clear_command();
	}
	if (input_chr == '\r') {
		return;
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
		_clear_command();
		_show_error();
		return false;
	}

	if (command_buffer[strlen(command_buffer)-1] == '\n') {
		command_buffer[strlen(command_buffer)-1] = 0;
		return true;
	}

	return false;
}

void _execute_command()
{
	char* command = strtok(command_buffer, " ");
	if (command == NULL) {
		goto do_error;
	}

	if (strncmp("status", command, CHAR_COMMAND_SIZE) == 0) {
		goto do_end;
	} else if (strncmp("saveadcmmin", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_ADC_min = get_liquid_adc();
		goto do_success;
	} else if (strncmp("saveadcmax", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_ADC_max = get_liquid_adc();
		goto do_success;
	}  else if (strncmp("default", command, CHAR_COMMAND_SIZE) == 0) {
		settings_reset();
	} else if (strncmp("clearlog", command, CHAR_COMMAND_SIZE) == 0) {
		clear_log();
		goto do_end;
	}


	char* value = strtok(NULL, " ");
	if (value == NULL) {
		goto do_error;
	}

	if (strncmp("setid", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.id = (uint32_t)atoi(value);
		goto do_success;
	} else if (strncmp("setsleep", command, CHAR_COMMAND_SIZE) == 0) {
		update_log_sleep(atoi(value) * MILLIS_IN_SECOND);
		goto do_success;
	} else if (strncmp("seturl", command, CHAR_COMMAND_SIZE) == 0) {
		strncpy(module_settings.server_url, value, sizeof(module_settings.server_url));
		goto do_success;
	} else if (strncmp("setport", command, CHAR_COMMAND_SIZE) == 0) {
		strncpy(module_settings.server_port, value, sizeof(module_settings.server_port));
		goto do_success;
	} else if (strncmp("setlitersmin", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_liters_min = atoi(value);
		goto do_success;
	} else if (strncmp("setlitersmax", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_liters_max = atoi(value);
		goto do_success;
	} else if (strncmp("settarget", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.milliliters_per_day = atoi(value);
		goto do_success;
	} else if (strncmp("setpumpspeed", command, CHAR_COMMAND_SIZE) == 0) {
		pump_update_speed((uint32_t)atoi(value));
		goto do_success;
	} else if (strncmp("setlogid", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.server_log_id = (uint32_t)atoi(value);
		goto do_success;
	}

	goto do_error;

do_success:
	module_settings.cf_id = 1;
	settings_save();
	goto do_end;

do_error:
	_show_error();
	goto do_end;

do_end:
	_show_status();
	pump_show_work();
	_clear_command();
}

void _clear_command()
{
	memset(command_buffer, 0, sizeof(command_buffer));
}

void _show_status()
{
	UART_MSG(SPRINTF_SETTINGS_FORMAT);
}

void _show_error()
{
	UART_MSG("Invalid UART command\n");
}
