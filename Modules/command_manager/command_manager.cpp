/*
 * uart_command.c
 *
 *  Created on: Sep 5, 2022
 *      Author: DrDeLaBill
 */

#include "command_manager.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "glog.h"
#include "soul.h"
#include "main.h"
#include "pump.h"
#include "clock.h"
#include "hal_defs.h"
#include "liquid_sensor.h"

#include "StorageAT.h"
#include "SettingsDB.h"
#include "LogService.h"


bool _validate_command();
void _execute_command();
void _clear_command();
void _show_error();


extern settings_t settings;
extern StorageAT storage;


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
		settings_show();
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
	bool isSuccess = false;
	if (command == NULL) {
		return;
	}

	if (strncmp("saveadcmin", command, CHAR_COMMAND_SIZE) == 0) {
		settings.tank_ADC_min = get_liquid_adc();
		isSuccess = true;
	} else if (strncmp("saveadcmax", command, CHAR_COMMAND_SIZE) == 0) {
		settings.tank_ADC_max = get_liquid_adc();
		isSuccess = true;
	}  else if (strncmp("default", command, CHAR_COMMAND_SIZE) == 0) {
		settings_reset(&settings);
		isSuccess = true;
	} else if (strncmp("clearlog", command, CHAR_COMMAND_SIZE) == 0) {
		LogService::clear();
		isSuccess = true;
	} else if (strncmp("clearpump", command, CHAR_COMMAND_SIZE) == 0) {
		pump_clear_log();
		isSuccess = true;
	} else if (strncmp("pump", command, CHAR_COMMAND_SIZE) == 0) {
		pump_show_status();
		isSuccess = true;
	} else if (strncmp("reset", command, CHAR_COMMAND_SIZE) == 0) {
		// TODO: очистка EEPROM
		isSuccess = false;
	}
#ifdef DEBUG
	else if (strncmp("format", command, CHAR_COMMAND_SIZE) == 0) {
		storage.format();
		isSuccess = true;
	}
#endif

	if (isSuccess) {
		set_status(NEED_SAVE_SETTINGS);
		_clear_command();
		settings_show();
		return;
	}

	char* value = strtok(NULL, " ");
	if (value == NULL) {
		settings_show();
		return;
	}

	if (strncmp("setsleep", command, CHAR_COMMAND_SIZE) == 0) {
		LogService::updateSleep(atoi(value) * MILLIS_IN_SECOND);
		isSuccess = true;
	} else if (strncmp("seturl", command, CHAR_COMMAND_SIZE) == 0) {
		strncpy(settings.url, value, sizeof(settings.url) - 1);
		isSuccess = true;
	} else if (strncmp("setlitersmin", command, CHAR_COMMAND_SIZE) == 0) {
		pump_update_ltrmin(atoi(value));
		isSuccess = true;
	} else if (strncmp("setlitersmax", command, CHAR_COMMAND_SIZE) == 0) {
		pump_update_ltrmax(atoi(value));
		isSuccess = true;
	} else if (strncmp("settarget", command, CHAR_COMMAND_SIZE) == 0) {
		pump_update_target(atoi(value));
		isSuccess = true;
	} else if (strncmp("setpumpspeed", command, CHAR_COMMAND_SIZE) == 0) {
		pump_update_speed(atoi(value));
		isSuccess = true;
	} else if (strncmp("setlogid", command, CHAR_COMMAND_SIZE) == 0) {
		settings.server_log_id = (uint32_t)atoi(value);
		isSuccess = true;
//	} else if (strncmp("delrecord", command, CHAR_COMMAND_SIZE) == 0) {
//		record_delete_record((uint32_t)atoi(value));
//		isSuccess = true;
	}  else if (strncmp("setpower", command, CHAR_COMMAND_SIZE) == 0) {
		pump_update_enable_state(atoi(value));
		isSuccess = true;
	}
#ifdef DEBUG
	else if (strncmp("setadcmin", command, CHAR_COMMAND_SIZE) == 0) {
		settings.tank_ADC_min = (uint32_t)atoi(value);
	    pump_reset_work_state();
		isSuccess = true;
	} else if (strncmp("setadcmax", command, CHAR_COMMAND_SIZE) == 0) {
		settings.tank_ADC_max = (uint32_t)atoi(value);
	    pump_reset_work_state();
		isSuccess = true;
	} else if (strncmp("setconfigver", command, CHAR_COMMAND_SIZE) == 0) {
		settings.cf_id = (uint32_t)atoi(value);
		isSuccess = true;
	}
#endif

	settings_show();

	if (isSuccess) {
		set_status(NEED_SAVE_SETTINGS);
		return;
	}

	_clear_command();
}

void _clear_command()
{
	memset(command_buffer, 0, sizeof(command_buffer));
}
