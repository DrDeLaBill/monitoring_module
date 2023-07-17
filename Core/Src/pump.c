/*
 * pump.c
 *
 *  Created on: Oct 27, 2022
 *      Author: georgy
 */

#include "pump.h"

#include <stdlib.h>
#include <stdbool.h>

#include "settings.h"
#include "command_manager.h"
#include "liquid_sensor.h"
#include "ds1307_for_stm32_hal.h"
#include "utils.h"


#define CYCLES_PER_HOUR    4
#define MONTHS_PER_YEAR    12
#define HOURS_PER_DAY      24
#define SECONDS_PER_MINUTE 60
#define MIN_PUMP_WORK_TIME 30000
#define MINUTES_PER_HOUR   60
#define LOG_SIZE           140
#define PUMP_WORK_PERIOD   900000


void _set_action(void (*action) (void));

void _work_state_action();
void _off_state_action();

void _calculate_work_time();
void _start_pump();
void _stop_pump();
void _watch_pump_work();
void _watch_pump_off();
void _write_work_time_to_log();
uint32_t _get_day_sec_left();


PumpState pump_state;


const char* PUMP_TAG = "\r\nPUMP";


void pump_init()
{
	_set_action(_work_state_action);
	pump_update_power(module_settings.pump_enabled);
	if (module_settings.milliliters_per_day == 0) {
		UART_MSG("No setting: milliliters_per_day\n");
	}
	if (module_settings.pump_speed == 0) {
		UART_MSG("No setting: pump_speed\n");
	}
	if (is_liquid_tank_empty()) {
		UART_MSG("Error liquid tank\n");
	}
}

void pump_proccess()
{
	pump_state.state_action();
}

void pump_update_speed(uint32_t speed)
{
	module_settings.pump_speed = speed;
	settings_save();
	pump_update_work();
}

void pump_update_work() {
	if (pump_state.state_action != _work_state_action) {
		return;
	}
	_calculate_work_time();
	pump_state.work_timer.delay = pump_state.needed_work_time;
}

void pump_update_power(bool enabled)
{
	module_settings.pump_enabled = enabled;
	HAL_GPIO_WritePin(STATE_LED_GPIO_Port, STATE_LED_Pin, (enabled ? SET : RESET));
}

void pump_show_work()
{
	uint32_t time_period = 0;
	if (pump_state.state == PUMP_WORK) {
		UART_MSG("Pump work from %lu to %lu\n", pump_state.start_time, pump_state.start_time + pump_state.needed_work_time);
		time_period = pump_state.needed_work_time;
	} else {
		time_period = pump_state.start_time - HAL_GetTick();
		UART_MSG("Pump will start at %lu\n", pump_state.start_time);
	}
	UART_MSG("Wait %lu min %lu sec\n", time_period / SECONDS_PER_MINUTE / MILLIS_IN_SECOND, (time_period / MILLIS_IN_SECOND) % SECONDS_PER_MINUTE);
	UART_MSG("Now: %lu\n", HAL_GetTick());
}

void clear_pump_log()
{
	module_settings.pump_downtime_sec = 0;
	module_settings.pump_work_sec = 0;
	module_settings.pump_work_day_sec = 0;
	settings_save();
}

void _set_action(void (*action) (void))
{
	pump_state.state_action = action;
	pump_state.state = PUMP_WAIT;
}

void _work_state_action()
{
	if (pump_state.state == PUMP_WAIT) {
		_calculate_work_time();
		_start_pump();
	}
	if (pump_state.state == PUMP_WORK) {
		_watch_pump_work();
	}
	if (pump_state.state == PUMP_OFF) {
		_set_action(_off_state_action);
	}
}

void _off_state_action()
{
	if (pump_state.state == PUMP_WAIT) {
		_stop_pump();
	}
	if (pump_state.state == PUMP_OFF) {
		_watch_pump_off();
	}
}

void _calculate_work_time()
{
	pump_state.needed_work_time = 0;
	if (module_settings.milliliters_per_day == 0) {
		return;
	}
	if (module_settings.pump_speed == 0) {
		return;
	}
	if (is_liquid_tank_empty()) {
		return;
	}
	volatile uint16_t used_day_liquid = module_settings.pump_work_day_sec * module_settings.pump_speed / MILLIS_IN_SECOND;
	if (module_settings.milliliters_per_day <= used_day_liquid) {
		return;
	}
	uint32_t time_left = _get_day_sec_left();
	uint32_t needed_ml = module_settings.milliliters_per_day - used_day_liquid;
	uint32_t max_pump_ml_to_end_of_day = module_settings.pump_speed * (time_left / (MINUTES_PER_HOUR * SECONDS_PER_MINUTE));

	if (needed_ml > max_pump_ml_to_end_of_day) {
		pump_state.needed_work_time = PUMP_WORK_PERIOD;
		return;
	}

	uint32_t periods_count = (time_left * MILLIS_IN_SECOND) / PUMP_WORK_PERIOD;
	uint32_t needed_ml_per_period = needed_ml / periods_count;
	pump_state.needed_work_time = (needed_ml_per_period * MILLIS_IN_SECOND / (module_settings.pump_speed / 4)) * MILLIS_IN_SECOND;

	if (pump_state.needed_work_time > PUMP_WORK_PERIOD) {
		pump_state.needed_work_time = PUMP_WORK_PERIOD;
	}
}

void _start_pump()
{
	if (pump_state.state == PUMP_WORK) {
		LOG_DEBUG(PUMP_TAG, " pump already work\n");
		return;
	}
	if (pump_state.needed_work_time < MIN_PUMP_WORK_TIME) {
		return;
	}
	pump_state.start_time = HAL_GetTick();
	Util_TimerStart(&pump_state.work_timer, pump_state.needed_work_time);
	if (module_settings.pump_enabled) {
		LOG_DEBUG(PUMP_TAG,	" PUMP ON (%ld ms)\n", pump_state.needed_work_time);
		pump_state.state = PUMP_WORK;
		HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, SET);
	} else {
		LOG_DEBUG(PUMP_TAG, " pump disabled\n");
		pump_state.state = PUMP_OFF;
	}
	pump_show_work();
}

void _stop_pump()
{
	if (pump_state.state == PUMP_OFF) {
		LOG_DEBUG(PUMP_TAG, " pump already off\n");
		return;
	}
	LOG_DEBUG(PUMP_TAG,	" PUMP OFF (%lu ms)\n", HAL_GetTick() - pump_state.start_time - pump_state.needed_work_time * MILLIS_IN_SECOND);
	pump_state.state = PUMP_OFF;
	HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, RESET);
	_write_work_time_to_log();
	pump_show_work();
}

void _write_work_time_to_log()
{
	if (!pump_state.needed_work_time) {
		return;
	}
	uint8_t cur_date = DS1307_GetDate();
	if (module_settings.pump_log_date != cur_date) {
		LOG_DEBUG(PUMP_TAG, " update day counter %d -> %d\n", module_settings.pump_log_date, cur_date);
		module_settings.pump_work_day_sec = 0;
		module_settings.pump_log_date = cur_date;
	}
	uint32_t time = pump_state.needed_work_time / MILLIS_IN_SECOND;
	if (module_settings.pump_enabled) {
		LOG_DEBUG(PUMP_TAG, " work time added (%ld ms)\n", time);
		module_settings.pump_work_day_sec += time;
		module_settings.pump_work_sec += time;
	} else {
		LOG_DEBUG(PUMP_TAG, " downtime added (%ld ms)\n", time);
		module_settings.pump_downtime_sec += time;
	}
	settings_save();
}

void _watch_pump_work()
{
	if (Util_TimerPending(&pump_state.work_timer)) {
		return;
	}
	pump_state.start_time += PUMP_WORK_PERIOD;
	if (pump_state.start_time < HAL_GetTick()) {
		_set_action(_work_state_action);
	} else {
		_set_action(_off_state_action);
	}
}

void _watch_pump_off()
{
	if (pump_state.start_time > HAL_GetTick()) {
		return;
	}
	_set_action(_work_state_action);
}

uint32_t _get_day_sec_left()
{
	return (SECONDS_PER_MINUTE - DS1307_GetSecond()) +
		   (MINUTES_PER_HOUR - DS1307_GetMinute()) * SECONDS_PER_MINUTE +
		   (HOURS_PER_DAY - DS1307_GetHour()) * SECONDS_PER_MINUTE * MINUTES_PER_HOUR;
}


void _remove_overage_time(uint16_t old_time)
{
	uint16_t remove_time = old_time - pump_state.needed_work_time;
	if (module_settings.pump_work_sec > remove_time) {
		module_settings.pump_work_sec -= remove_time;
	} else {
		module_settings.pump_work_sec = 0;
	}
	if (module_settings.pump_work_day_sec > remove_time) {
		module_settings.pump_work_day_sec -= remove_time;
	} else {
		module_settings.pump_work_day_sec = 0;
	}
}

void _add_extra_time(uint16_t old_time)
{
	uint16_t add_time = pump_state.needed_work_time - old_time;
	module_settings.pump_work_sec += add_time;
	module_settings.pump_work_day_sec += add_time;
}
