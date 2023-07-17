/*
 * functions.h
 *
 *  Created on: Sep 4, 2022
 *      Author: georg
 */

#ifndef INC_FUNCTIONS_H_
#define INC_FUNCTIONS_H_

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include "settings_manager.h"
#include "ds1307_for_stm32_hal.h"


#define DEFAULT_SLEEPING_TIME 900000

#define SPRINTF_SETTINGS_FORMAT "\nTime: %u-%02u-%02uT%02u:%02u:%02u\n" \
								"Device ID: %lu\n" \
								"Server URL: %s:%s\n" \
								"ADC level MIN: %u\n" \
								"ADC level MAX: %u\n" \
								"Liquid level MIN: %u l\n" \
								"Liquid level MAX: %u l\n" \
								"Target: %lu ml/d\n" \
								"Pump speed: %lu ml/h\n" \
								"Sleep time: %lu sec\n" \
								"Server log ID: %lu\n" \
								"Pump work: %lu sec\n" \
								"Pump work day: %lu sec\n" \
								"Config ver: %lu\n" \
								"Power %s\n\n", \
								DS1307_GetYear(), \
								DS1307_GetMonth(), \
								DS1307_GetDate(), \
								DS1307_GetHour(), \
								DS1307_GetMinute(), \
								DS1307_GetSecond(), \
								module_settings.id, \
								module_settings.server_url, \
								module_settings.server_port, \
								module_settings.tank_ADC_min, \
								module_settings.tank_ADC_max, \
								module_settings.tank_liters_min, \
								module_settings.tank_liters_max, \
								module_settings.milliliters_per_day, \
								module_settings.pump_speed, \
								module_settings.sleep_time / MILLIS_IN_SECOND, \
								module_settings.server_log_id, \
								module_settings.pump_work_sec, \
								module_settings.pump_work_day_sec, \
								module_settings.cf_id, \
								module_settings.pump_enabled ? "ON" : "OFF"


extern settings_tag_t general_settings_load;
extern ModuleSettings module_settings;

void show_settings();
int _write(int file, uint8_t *ptr, int len);

#endif /* INC_FUNCTIONS_H_ */
