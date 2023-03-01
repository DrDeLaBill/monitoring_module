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
#include "pump.h"


bool _validate_command();
void _execute_command();
void _clear_command();
void _clear_command();
char* _get_value();

void _cmd_action_status();
void _cmd_action_time();
void _cmd_action_default();
void _cmd_action_clearlog();
void _cmd_action_setid();
void _cmd_action_setsleep();
void _cmd_action_seturl();
void _cmd_action_setport();
void _cmd_action_setlitersmin();
void _cmd_action_setlitersmax();
void _cmd_action_saveadcmin();
void _cmd_action_saveadcmax();
void _cmd_action_settarget();
void _cmd_action_setpumpspeed();
void _cmd_action_setlogid();


const char* COMMAND_TAG = "CMND";

const char* CMD_STATUS     = "status";
const char* CMD_DEFAULT      = "default";
const char* CMD_CLEARLOG     = "clearlog";
const char* CMD_SETID        = "setid";
const char* CMD_SETSLEEP     = "setsleep";
const char* CMD_SETURL       = "seturl";
const char* CMD_SETPORT      = "setport";
const char* CMD_SETLITERSMIN = "setlitersmin";
const char* CMD_SETLITERSMAX = "setlitersmax";
const char* CMD_SAVEADCMIN   = "saveadcmin";
const char* CMD_SAVEADCMAX   = "saveadcmax";
const char* CMD_SETTARGET    = "settarget";
const char* CMD_SETPUMPSPEED = "setpumpspeed";
const char* CMD_SETLOGID     = "setlogid";

const char* MSG_INVALID_LITERS = "Invalid liters value\n";

char command_buffer[CHAR_COMMAND_SIZE] = {};

const cmd_state command_states[] = {
	{&CMD_STATUS, &_cmd_action_status},
	{&CMD_DEFAULT, &_cmd_action_default},
	{&CMD_CLEARLOG, &_cmd_action_clearlog},
	{&CMD_SETID, &_cmd_action_setid},
	{&CMD_SETSLEEP, &_cmd_action_setsleep},
	{&CMD_SETURL, &_cmd_action_seturl},
	{&CMD_SETPORT, &_cmd_action_setport},
	{&CMD_SETLITERSMIN, &_cmd_action_setlitersmin},
	{&CMD_SETLITERSMAX, &_cmd_action_setlitersmax},
	{&CMD_SAVEADCMIN, &_cmd_action_saveadcmin},
	{&CMD_SAVEADCMAX, &_cmd_action_saveadcmax},
	{&CMD_SETTARGET, &_cmd_action_settarget},
	{&CMD_SETPUMPSPEED, &_cmd_action_setpumpspeed},
	{&CMD_SETLOGID, &_cmd_action_setlogid}
};


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
		return false;
	}

	char symbol = command_buffer[strlen(command_buffer)-1];
	if (symbol == '\n') {
		command_buffer[strlen(command_buffer)-1] = 0;
		return true;
	}

	return false;
}

void _execute_command()
{
	char* command = strtok(command_buffer, " ");
	for (uint8_t i = 0; i < sizeof(command_states) / sizeof(cmd_state); i++) {
		if (!strncmp(
				*command_states[i].cmd_name,
				command,
				strlen(*command_states[i].cmd_name
		))) {
			command_states[i].cmd_action();
			goto do_end;
		}
	}

	UART_MSG("Invalid UART command\n");
	goto do_end;

do_end:
	_clear_command();
}

void _clear_command()
{
	memset(command_buffer, 0, sizeof(command_buffer));
}

char* _get_value()
{
	return strtok(NULL, " ");
}

void _cmd_action_status()
{
	UART_MSG(SPRINTF_SETTINGS_FORMAT);
	pump_show_work();
}

void _cmd_action_default()
{
	settings_reset();
	UART_MSG(SPRINTF_SETTINGS_FORMAT);
}

void _cmd_action_clearlog()
{
	clear_log();
}

void _cmd_action_setid()
{
	char* value = _get_value();
	if (value == NULL) {
		UART_MSG("Invalid id\n");
		return;
	}
	module_settings.id = (uint32_t)atoi(value);
	settings_save();
}

void _cmd_action_setsleep()
{
	char* value = _get_value();
	if (value == NULL) {
		UART_MSG("Invalid time\n");
		return;
	}
	update_log_sleep( atoi(value) * MILLIS_IN_SECOND);
	settings_save();
}

void _cmd_action_seturl()
{
	char* value = _get_value();
	if (value == NULL) {
		UART_MSG("Invalid URL\n");
		return;
	}
	strncpy(module_settings.server_url, value, sizeof(module_settings.server_url));
	settings_save();
}

void _cmd_action_setport()
{
	char* value = _get_value();
	if (value == NULL) {
		UART_MSG("Invalid port\n");
		return;
	}
	strncpy(module_settings.server_port, value, sizeof(module_settings.server_port));
	settings_save();
}

void _cmd_action_setlitersmin()
{
	char* value = _get_value();
	if (value == NULL) {
		UART_MSG(MSG_INVALID_LITERS);
		return;
	}
	module_settings.tank_liters_min = atoi(value);
	settings_save();
}

void _cmd_action_setlitersmax()
{
	char* value = _get_value();
	if (value == NULL) {
		UART_MSG(MSG_INVALID_LITERS);
		return;
	}
	module_settings.tank_liters_max = atoi(value);
	settings_save();
}

void _cmd_action_saveadcmin()
{
	module_settings.tank_ADC_min = get_liquid_adc();
	settings_save();
}

void _cmd_action_saveadcmax()
{
	module_settings.tank_ADC_max = get_liquid_adc();
	settings_save();
}

void _cmd_action_settarget()
{
	char* value = _get_value();
	if (value == NULL) {
		UART_MSG(MSG_INVALID_LITERS);
		return;
	}
	module_settings.milliliters_per_day = atoi(value);
	settings_save();
}

void _cmd_action_setpumpspeed()
{
	char* value = _get_value();
	if (value == NULL) {
		UART_MSG(MSG_INVALID_LITERS);
		return;
	}
	pump_update_speed((uint32_t)atoi(value));
	settings_save();
}

void _cmd_action_setlogid()
{
	char* value = _get_value();
	if (value == NULL) {
		UART_MSG("Invalid log ID\n");
		return;
	}
	module_settings.server_log_id = (uint32_t)atoi(value);
	settings_save();
}
