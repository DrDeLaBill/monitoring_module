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

#include "Log.h"
#include "main.h"
#include "pump.h"
#include "clock.h"
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
		set_settings_update_status(true);
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
		settings.id = (uint32_t)atoi(value);
		isSuccess = true;
	} else if (strncmp("setsleep", command, CHAR_COMMAND_SIZE) == 0) {
		LogService::updateSleep(atoi(value) * MILLIS_IN_SECOND);
		isSuccess = true;
	} else if (strncmp("seturl", command, CHAR_COMMAND_SIZE) == 0) {
		strncpy(settings.server_url, value, sizeof(settings.server_url) - 1);
		isSuccess = true;
	} else if (strncmp("setport", command, CHAR_COMMAND_SIZE) == 0) {
		strncpy(settings.server_port, value, sizeof(settings.server_port) - 1);
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

	if (isSuccess) {
		set_settings_update_status(true);
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
	printPretty(
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
		clock_get_year() % 100, \
		clock_get_month(), \
		clock_get_date(), \
		clock_get_hour(), \
		clock_get_minute(), \
		clock_get_second(), \
		settings.id, \
		settings.server_url, \
		settings.server_port, \
		settings.tank_ADC_min, \
		settings.tank_ADC_max, \
		settings.tank_liters_min / MILLILITERS_IN_LITER, \
		settings.tank_liters_max / MILLILITERS_IN_LITER, \
		settings.pump_target / MILLILITERS_IN_LITER, \
		settings.sleep_time / MILLIS_IN_SECOND, \
		settings.server_log_id, \
		settings.pump_speed, \
		settings.pump_work_sec, \
		settings.pump_work_day_sec, \
		settings.cf_id, \
		settings.pump_enabled ? "ON" : "OFF"
	);
}
