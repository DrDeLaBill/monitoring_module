/*
 * uart_command.c
 *
 *  Created on: Sep 5, 2022
 *      Author: DrDeLaBill
 */

#include <command_manager.h>

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "liquid_sensor.h"
#include "utils.h"
#include "pump.h"

#include "StorageAT.h"
#include "SettingsDB.h"
#include "LogService.h"


bool _validate_command();
void _execute_command();
void _clear_command();
void _show_error();


extern SettingsDB settings;
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
	bool isSuccess = false;
	if (command == NULL) {
		_show_error();
		return;
	}

	if (strncmp("saveadcmin", command, CHAR_COMMAND_SIZE) == 0) {
		settings.settings.tank_ADC_min = get_liquid_adc();
		isSuccess = true;
	} else if (strncmp("saveadcmax", command, CHAR_COMMAND_SIZE) == 0) {
		settings.settings.tank_ADC_max = get_liquid_adc();
		isSuccess = true;
	}  else if (strncmp("default", command, CHAR_COMMAND_SIZE) == 0) {
		settings.reset();
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
		settings.save();
		_show_error();
		_clear_command();
		return;
	}

	char* value = strtok(NULL, " ");
	if (value == NULL) {
		_show_error();
		return;
	}

	if (strncmp("setid", command, CHAR_COMMAND_SIZE) == 0) {
		settings.settings.id = (uint32_t)atoi(value);
		isSuccess = true;
	} else if (strncmp("setsleep", command, CHAR_COMMAND_SIZE) == 0) {
		LogService::updateSleep(atoi(value) * MILLIS_IN_SECOND);
		isSuccess = true;
	} else if (strncmp("seturl", command, CHAR_COMMAND_SIZE) == 0) {
		strncpy(settings.settings.server_url, value, sizeof(settings.settings.server_url) - 1);
		isSuccess = true;
	} else if (strncmp("setport", command, CHAR_COMMAND_SIZE) == 0) {
		strncpy(settings.settings.server_port, value, sizeof(settings.settings.server_port) - 1);
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
		settings.settings.server_log_id = (uint32_t)atoi(value);
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
		settings.settings.tank_ADC_min = (uint32_t)atoi(value);
	    pump_reset_work_state();
		isSuccess = true;
	} else if (strncmp("setadcmax", command, CHAR_COMMAND_SIZE) == 0) {
		settings.settings.tank_ADC_max = (uint32_t)atoi(value);
	    pump_reset_work_state();
		isSuccess = true;
	} else if (strncmp("setconfigver", command, CHAR_COMMAND_SIZE) == 0) {
		settings.settings.cf_id = (uint32_t)atoi(value);
		isSuccess = true;
	}
#endif

	if (isSuccess) {
		settings.save();
		return;
	}

	_show_error();
	_clear_command();
}

void _clear_command()
{
	memset(command_buffer, 0, sizeof(command_buffer));
}

void _show_error()
{
	PRINT_MESSAGE(
		COMMAND_TAG,
		"\n\n####################SETTINGS####################\n" \
		"Time:             20%02u-%02u-%02uT%02u:%02u:%02u\n" \
		"Device ID:        %lu\n" \
		"Server URL:       %s:%s\n" \
		"ADC level MIN:    %lu\n" \
		"ADC level MAX:    %lu\n" \
		"Liquid level MIN: %lu l\n" \
		"Liquid level MAX: %lu l\n" \
		"Target:           %lu l/d\n" \
		"Sleep time:       %lu sec\n" \
		"Server log ID:    %lu\n" \
		"Pump speed:       %lu ml/h\n" \
		"Pump work:        %lu sec\n" \
		"Pump work day:    %lu sec\n" \
		"Config ver:       %lu\n" \
		"Pump              %s\n" \
		"####################SETTINGS####################\n\n", \
		get_year() % 100, \
		get_month(), \
		get_date(), \
		get_hour(), \
		get_minute(), \
		get_second(), \
		settings.settings.id, \
		settings.settings.server_url, \
		settings.settings.server_port, \
		settings.settings.tank_ADC_min, \
		settings.settings.tank_ADC_max, \
		settings.settings.tank_liters_min / MILLILITERS_IN_LITER, \
		settings.settings.tank_liters_max / MILLILITERS_IN_LITER, \
		settings.settings.pump_target / MILLILITERS_IN_LITER, \
		settings.settings.sleep_time / MILLIS_IN_SECOND, \
		settings.settings.server_log_id, \
		settings.settings.pump_speed, \
		settings.settings.pump_work_sec, \
		settings.settings.pump_work_day_sec, \
		settings.settings.cf_id, \
		settings.settings.pump_enabled ? "ON" : "OFF"
	);
}
