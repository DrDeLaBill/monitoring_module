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

#include "logger.h"
#include "storage_data_manager.h"
#include "settings_manager.h"
#include "record_manager.h"
#include "liquid_sensor.h"
#include "utils.h"
#include "pump.h"


bool _validate_command();
void _execute_command();
void _clear_command();
void _show_error();


const char *COMMAND_TAG = "UART";
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
		goto do_show_end;
	} else if (strncmp("saveadcmin", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_ADC_min = get_liquid_adc();
		goto do_success;
	} else if (strncmp("saveadcmax", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_ADC_max = get_liquid_adc();
		goto do_success;
	}  else if (strncmp("default", command, CHAR_COMMAND_SIZE) == 0) {
		settings_reset();
	} else if (strncmp("clearlog", command, CHAR_COMMAND_SIZE) == 0) {
		clear_log();
		goto do_show_end;
	} else if (strncmp("clearpump", command, CHAR_COMMAND_SIZE) == 0) {
		pump_clear_log();
		goto do_show_end;
	} else if (strncmp("pump", command, CHAR_COMMAND_SIZE) == 0) {
		pump_show_status();
		goto do_clear;
	} else if (strncmp("reset", command, CHAR_COMMAND_SIZE) == 0) {
		// TODO: очистка EEPROM
		goto do_clear;
	}
#ifdef DEBUG
	else if (strncmp("reseteepromerr", command, CHAR_COMMAND_SIZE) == 0) {
		storage_reset_errors_list_page();
		goto do_success;
	}
#endif


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
		pump_reset_work_state();
		goto do_success;
	} else if (strncmp("setpumpspeed", command, CHAR_COMMAND_SIZE) == 0) {
		pump_update_speed((uint32_t)atoi(value));
		goto do_success;
	} else if (strncmp("setlogid", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.server_log_id = (uint32_t)atoi(value);
		goto do_success;
	}
#ifdef DEBUG
	else if (strncmp("setadcmin", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_ADC_min = (uint32_t)atoi(value);
		goto do_success;
	} else if (strncmp("setadcmax", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.tank_ADC_max = (uint32_t)atoi(value);
		goto do_success;
	} else if (strncmp("setpower", command, CHAR_COMMAND_SIZE) == 0) {
		bool enabled = (bool)atoi(value);
		module_settings.pump_enabled = enabled;
		pump_update_enable_state(enabled);
		goto do_success;
	} else if (strncmp("setconfigver", command, CHAR_COMMAND_SIZE) == 0) {
		module_settings.cf_id = (uint32_t)atoi(value);
		goto do_success;
	}
#endif

	goto do_error;

do_success:
//	module_settings.cf_id = 1;
	settings_save();
	goto do_show_end;

do_error:
	_show_error();
	goto do_show_end;

do_show_end:
	LOG_MESSAGE(COMMAND_TAG, LOG_DEBUG_SETTINGS_FORMAT);
	goto do_clear;

do_clear:
	_clear_command();
}

void _clear_command()
{
	memset(command_buffer, 0, sizeof(command_buffer));
}

void _show_error()
{
	LOG_MESSAGE(COMMAND_TAG, "invalid UART command\n");
}
