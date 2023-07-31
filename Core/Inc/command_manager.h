/*
 * uart_command.h
 *
 *  Created on: Sep 5, 2022
 *      Author: DrDeLaBill
 */

#ifndef INC_COMMAND_MANAGER_H_
#define INC_COMMAND_MANAGER_H_

#include <stdbool.h>

#include "clock_service.h"

#define CHAR_COMMAND_SIZE  40
#define UART_RESPONSE_SIZE 40


void command_manager_begin();
void command_manager_proccess();
void cmd_proccess_input(const char input_chr);


#define LOG_DEBUG_SETTINGS_FORMAT \
		"\n###################SETTINGS###################\n" \
		"Time:             20%02u-%02u-%02uT%02u:%02u:%02u\n" \
		"Device ID:        %lu\n" \
		"Server URL:       %s:%s\n" \
		"ADC level MIN:    %lu\n" \
		"ADC level MAX:    %lu\n" \
		"Liquid level MIN: %lu l\n" \
		"Liquid level MAX: %lu l\n" \
		"Target:           %lu ml/d\n" \
		"Sleep time:       %lu sec\n" \
		"Server log ID:    %lu\n" \
		"Pump speed:       %lu ml/h\n" \
		"Pump work:        %lu sec\n" \
		"Pump work day:    %lu sec\n" \
		"Config ver:       %lu\n" \
		"Pump              %s\n" \
		"###################SETTINGS###################\n", \
		get_year(), \
		get_month(), \
		get_date(), \
		get_hour(), \
		get_minute(), \
		get_second(), \
		module_settings.id, \
		module_settings.server_url, \
		module_settings.server_port, \
		module_settings.tank_ADC_min, \
		module_settings.tank_ADC_max, \
		module_settings.tank_liters_min, \
		module_settings.tank_liters_max, \
		module_settings.milliliters_per_day, \
		module_settings.sleep_time / MILLIS_IN_SECOND, \
		module_settings.server_log_id, \
		module_settings.pump_speed, \
		module_settings.pump_work_sec, \
		module_settings.pump_work_day_sec, \
		module_settings.cf_id, \
		module_settings.pump_enabled ? "ON" : "OFF"


#endif /* INC_COMMAND_MANAGER_H_ */
