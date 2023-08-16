/*
 * pump.c
 *
 *  Created on: Oct 27, 2022
 *      Author: georgy
 */

#include "pump.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "command_manager.h"
#include "liquid_sensor.h"
#include "utils.h"
#include "settings_manager.h"
#include "clock_service.h"
#include "pressure_sensor.h"


#define CYCLES_PER_HOUR    4
#define MONTHS_PER_YEAR    12
#define HOURS_PER_DAY      24
#define SECONDS_PER_MINUTE 60
#define MIN_PUMP_WORK_TIME ((uint32_t)30000)
#define PUMP_OFF_TIME_MIN  ((uint32_t)5000)
#define MINUTES_PER_HOUR   60
#define LOG_SIZE           140
#define PUMP_WORK_PERIOD   ((uint32_t)900000)

#define PUMP_LED_DISABLE_STATE_OFF_TIME ((uint32_t)6000)
#define PUMP_LED_DISABLE_STATE_ON_TIME  ((uint32_t)300)
#define PUMP_LED_WORK_STATE_PERIOD      ((uint32_t)1000)


void _pump_set_state(void (*action) (void));

void _pump_fsm_state_enable();
void _pump_fsm_state_start();
void _pump_fsm_state_stop();
void _pump_fsm_state_work();
void _pump_fsm_state_check_downtime();
void _pump_fsm_state_off();
void _pump_fsm_state_write_log();

void _pump_clear_state();
void _pump_log_work_time();
void _pump_log_downtime();
void _pump_check_log_date();

void _pump_calculate_work_time();
void _pump_indication_proccess();
void _pump_indicate_disable_state();
void _pump_indicate_work_state();
uint32_t _get_day_sec_left();


pump_state_t pump_state;


const char* PUMP_TAG = "PUMP";


void pump_init()
{
	_pump_clear_state();

	pump_update_enable_state(module_settings.pump_enabled);

    if (module_settings.pump_target == 0) {
        LOG_MESSAGE(PUMP_TAG, "WWARNING - pump init - no setting milliliters_per_day\n");
    }
    if (module_settings.pump_speed == 0) {
        LOG_MESSAGE(PUMP_TAG, "WWARNING - pump init - no setting pump_speed\n");
    }
    if (is_liquid_tank_empty()) {
        LOG_MESSAGE(PUMP_TAG, "WWARNING - pump init - liquid tank empty\n");
    }
}

void pump_proccess()
{
    pump_state.state_action();
    _pump_indication_proccess();
}

void pump_update_speed(uint32_t speed)
{
	if (speed == module_settings.pump_speed) {
		return;
	}

    module_settings.pump_speed = speed;

    pump_reset_work_state();
}

void pump_update_enable_state(bool enabled)
{
	module_settings.pump_enabled = enabled;

	if (pump_state.enabled == enabled) {
		return;
	}

	pump_reset_work_state();

	pump_state.enabled = enabled;
}

void pump_update_ltrmin(uint32_t ltrmin)
{
	ltrmin *= MILLILITERS_IN_LITER;

	if (ltrmin == module_settings.tank_liters_min) {
		return;
	}

	module_settings.tank_liters_min = ltrmin;

	pump_reset_work_state();
}

void pump_update_ltrmax(uint32_t ltrmax)
{
	ltrmax *= MILLILITERS_IN_LITER;

	if (ltrmax == module_settings.tank_liters_max) {
		return;
	}

	module_settings.tank_liters_max = ltrmax;

	pump_reset_work_state();
}

void pump_update_target(uint32_t target)
{
	target *= MILLILITERS_IN_LITER;

	if (target == module_settings.pump_target) {
		return;
	}

	module_settings.pump_target = target;

	pump_reset_work_state();
}

void pump_reset_work_state() {
	if (pump_state.state_action == _pump_fsm_state_work) {
		_pump_log_work_time();
	} else if (pump_state.state_action == _pump_fsm_state_check_downtime) {
		_pump_log_downtime();
	}

	_pump_clear_state();
}

void pump_clear_log()
{
    module_settings.pump_downtime_sec = 0;
    module_settings.pump_work_sec = 0;
    module_settings.pump_work_day_sec = 0;
	_pump_clear_state();
    settings_save();
}

void pump_show_status()
{
	int32_t liquid_val = get_liquid_liters();
	uint16_t liquid_adc = get_liquid_adc();
	uint16_t pressure_1 = get_press();

    LOG_MESSAGE(PUMP_TAG, "PUMP_STATUS: %s\n################################################\n", pump_state.enabled ? "ENABLED" : "DISABLED");

    uint16_t used_day_liquid = module_settings.pump_work_day_sec * module_settings.pump_speed / MILLIS_IN_SECOND;
    if (module_settings.pump_target == 0) {
		LOG_MESSAGE(PUMP_TAG, "Unable to calculate work time - no setting day liquid target\n");
	} else if (module_settings.pump_speed == 0) {
		LOG_MESSAGE(PUMP_TAG, "Unable to calculate work time - no setting pump speed\n");
	} else if (is_liquid_tank_empty()) {
		LOG_MESSAGE(PUMP_TAG, "Unable to calculate work time - liquid tank empty\n");
	} else if (pump_state.needed_work_time < MIN_PUMP_WORK_TIME) {
    	LOG_MESSAGE(PUMP_TAG, "Unable to calculate work time - needed work time less than %lu sec; set work time 0 sec\n", MIN_PUMP_WORK_TIME / 1000);
	} else if (module_settings.pump_target <= used_day_liquid) {
		LOG_MESSAGE(PUMP_TAG, "Unable to calculate work time - target liquid amount per day already used\n");
	}

    uint32_t time_period = 0;
    if (!module_settings.pump_enabled || (!pump_state.needed_work_time && !pump_state.start_time)) {
		LOG_MESSAGE(PUMP_TAG, "Pump will not start - unexceptable settings or sesnors values\n");
		LOG_MESSAGE(PUMP_TAG, "Please check settings: target liters per day, tank ADC values, tank liters values or enable state\n");
	} else if (pump_state.state_action == _pump_fsm_state_start || pump_state.state_action == _pump_fsm_state_work) {
        time_period = (pump_state.start_time + pump_state.needed_work_time) - HAL_GetTick();
        LOG_MESSAGE(PUMP_TAG, "Pump work from %lu ms to %lu ms (internal)\n", pump_state.start_time, pump_state.start_time + pump_state.needed_work_time);
    } else if (pump_state.state_action == _pump_fsm_state_stop || pump_state.state_action == _pump_fsm_state_off) {
    	time_period = pump_state.start_time + PUMP_WORK_PERIOD - HAL_GetTick();
        LOG_MESSAGE(PUMP_TAG, "Pump will start at %lu ms (internal)\n", HAL_GetTick() - pump_state.start_time + PUMP_WORK_PERIOD);
    } else if (pump_state.state_action == _pump_fsm_state_check_downtime) {
    	LOG_MESSAGE(PUMP_TAG, "Counting pump downtime period\n");
    } else {
    	LOG_MESSAGE(PUMP_TAG, "Pump current day work time: %lu\n", module_settings.pump_work_day_sec);
	}

    if (time_period) {
    	LOG_MESSAGE(PUMP_TAG, "Wait %lu min %lu sec\n", time_period / SECONDS_PER_MINUTE / MILLIS_IN_SECOND, (time_period / MILLIS_IN_SECOND) % SECONDS_PER_MINUTE);
    }

    LOG_MESSAGE(PUMP_TAG, "Internal clock: %lu ms\n", HAL_GetTick());

    LOG_MESSAGE(PUMP_TAG, "Liquid pressure: %u.%02u MPa\n", pressure_1 / 100, pressure_1 % 100);

    if (liquid_val < 0) {
    	LOG_MESSAGE(PUMP_TAG, "Tank liquid value ERR (ADC=%d)\n################################################\n", liquid_adc);
    } else {
    	LOG_MESSAGE(PUMP_TAG, "Tank liquid value: %ld l (ADC=%d)\n################################################\n", liquid_val, liquid_adc);
    }
}

void _pump_indication_proccess()
{
	if (!pump_state.enabled) {
		_pump_indicate_disable_state();
		return;
	}

	if (pump_state.state_action == _pump_fsm_state_start || pump_state.state_action == _pump_fsm_state_work) {
		_pump_indicate_work_state();
	} else {
		HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(PUMP_LAMP_GPIO_Port, PUMP_LAMP_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
	}
}

void _pump_indicate_disable_state()
{
	HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);

	if (util_is_timer_wait(&pump_state.indication_timer)) {
		return;
	}

	GPIO_PinState state = HAL_GPIO_ReadPin(RED_LED_GPIO_Port, RED_LED_Pin);

	if (state) {
		util_timer_start(&pump_state.indication_timer, PUMP_LED_DISABLE_STATE_OFF_TIME);
	} else {
		util_timer_start(&pump_state.indication_timer, PUMP_LED_DISABLE_STATE_ON_TIME);
	}

	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, !state);
	HAL_GPIO_WritePin(PUMP_LAMP_GPIO_Port, PUMP_LAMP_Pin, !state);
}

void _pump_indicate_work_state()
{
	if (util_is_timer_wait(&pump_state.indication_timer)) {
		return;
	}

	GPIO_PinState state = HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);

	util_timer_start(&pump_state.indication_timer, PUMP_LED_WORK_STATE_PERIOD);

	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, state);
	HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, !state);
	HAL_GPIO_WritePin(PUMP_LAMP_GPIO_Port, PUMP_LAMP_Pin, !state);
}

void _pump_set_state(void (*action) (void))
{
    pump_state.state_action = action;
}

void _pump_clear_state()
{
	memset((uint8_t*)&pump_state, 0, sizeof(pump_state));
	pump_state.enabled = module_settings.pump_enabled;
	_pump_set_state(_pump_fsm_state_enable);
}

void _pump_fsm_state_enable()
{
	_pump_clear_state();
	_pump_check_log_date();
	_pump_calculate_work_time();
	if (pump_state.needed_work_time) {
		_pump_set_state(_pump_fsm_state_start);
	} else {
		_pump_set_state(_pump_fsm_state_stop);
	}
}

void _pump_fsm_state_start()
{
	pump_state.start_time = HAL_GetTick();
	util_timer_start(&pump_state.wait_timer, pump_state.needed_work_time);

	if (!module_settings.pump_enabled) {
		LOG_DEBUG(PUMP_TAG, "PUMP COUNT DOWNTIME (wait %lu ms)\n", pump_state.needed_work_time);
	    _pump_set_state(_pump_fsm_state_check_downtime);
	    pump_show_status();
	    return;
	}

	LOG_DEBUG(PUMP_TAG, "PUMP ON (will work %lu ms)\n", pump_state.needed_work_time);
	HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, GPIO_PIN_SET);

    _pump_set_state(_pump_fsm_state_work);

    pump_show_status();
}

void _pump_fsm_state_stop()
{
	uint32_t work_state_time = (HAL_GetTick() - pump_state.start_time);
	if (work_state_time > PUMP_WORK_PERIOD) {
		HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, GPIO_PIN_RESET);
		_pump_set_state(_pump_fsm_state_enable);
		return;
	}

	uint32_t off_state_time = PUMP_WORK_PERIOD - work_state_time;
	if (off_state_time < MIN_PUMP_WORK_TIME) {
		_pump_set_state(_pump_fsm_state_enable);
		return;
	}

	LOG_DEBUG(PUMP_TAG, "PUMP OFF (%lu ms)\n", off_state_time);

	util_timer_start(&pump_state.wait_timer, off_state_time);
	HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, GPIO_PIN_RESET);

    _pump_set_state(_pump_fsm_state_off);

    pump_show_status();
}

void _pump_fsm_state_work()
{
	if (is_liquid_tank_empty()) {
		goto do_pump_stop;
	}

	if (util_is_timer_wait(&pump_state.wait_timer)) {
		return;
	}

do_pump_stop:

	_pump_log_work_time();

	_pump_set_state(_pump_fsm_state_stop);
}

void _pump_fsm_state_check_downtime()
{
	if (util_is_timer_wait(&pump_state.wait_timer)) {
		return;
	}

	_pump_log_downtime();

	_pump_set_state(_pump_fsm_state_stop);
}

void _pump_fsm_state_off()
{
	if (util_is_timer_wait(&pump_state.wait_timer)) {
		return;
	}

	_pump_set_state(_pump_fsm_state_enable);

    pump_show_status();
}

void _pump_log_work_time()
{
	_pump_check_log_date();

	if (pump_state.state_action != _pump_fsm_state_work) {
		return;
	}

	if (HAL_GetTick() < pump_state.start_time) {
		return;
	}

	uint32_t work_state_time = __abs_dif(HAL_GetTick(), pump_state.start_time);
	if (work_state_time < MILLIS_IN_SECOND) {
		return;
	}

	uint32_t time = work_state_time / MILLIS_IN_SECOND;
	module_settings.pump_work_day_sec += time;
	module_settings.pump_work_sec += time;
	LOG_DEBUG(PUMP_TAG, "update work log: time added (%lu s)\n", time);

    settings_save();
}

void _pump_log_downtime()
{
	_pump_check_log_date();

	if (pump_state.state_action != _pump_fsm_state_check_downtime) {
		return;
	}

	if (HAL_GetTick() < pump_state.start_time) {
		return;
	}

	uint32_t time = __abs_dif(HAL_GetTick(), pump_state.start_time) / MILLIS_IN_SECOND;
    module_settings.pump_downtime_sec += time;
    LOG_DEBUG(PUMP_TAG, "update downtime log: time added (%ld s)\n", time);

    settings_save();
}

void _pump_check_log_date()
{
	uint8_t cur_date = get_date();
	if (module_settings.pump_log_date != cur_date) {
		LOG_DEBUG(PUMP_TAG, "update pump log: day counter - %u -> %u\n", module_settings.pump_log_date, cur_date);
		module_settings.pump_work_day_sec = 0;
		module_settings.pump_log_date = cur_date;
	}
}

void _pump_calculate_work_time()
{
    pump_state.needed_work_time = 0;

    if (module_settings.pump_target == 0) {
    	return;
    }
    if (module_settings.pump_speed == 0) {
    	return;
    }
    if (is_liquid_tank_empty()) {
    	return;
    }

    uint16_t used_day_liquid = (module_settings.pump_work_day_sec * module_settings.pump_speed) / (MINUTES_PER_HOUR * SECONDS_PER_MINUTE);
    if (module_settings.pump_target <= used_day_liquid) {
    	return;
    }

    uint32_t time_left = _get_day_sec_left();
    uint32_t needed_ml = module_settings.pump_target - used_day_liquid;
    uint32_t max_pump_ml_to_end_of_day = (module_settings.pump_speed * time_left) / (MINUTES_PER_HOUR * SECONDS_PER_MINUTE);
    if (needed_ml > max_pump_ml_to_end_of_day) {
        pump_state.needed_work_time = PUMP_WORK_PERIOD;
        return;
    }

    uint32_t periods_count = (time_left * MILLIS_IN_SECOND) / PUMP_WORK_PERIOD;
    uint32_t needed_ml_per_period = needed_ml / periods_count;
    pump_state.needed_work_time = (needed_ml_per_period * (MINUTES_PER_HOUR * SECONDS_PER_MINUTE)) / (module_settings.pump_speed);
    pump_state.needed_work_time *= MILLIS_IN_SECOND;
    if (pump_state.needed_work_time > PUMP_WORK_PERIOD) {
        pump_state.needed_work_time = PUMP_WORK_PERIOD;
    }
	if (pump_state.needed_work_time < MIN_PUMP_WORK_TIME) {
    	pump_state.needed_work_time = 0;
	}
}

uint32_t _get_day_sec_left()
{
    return (SECONDS_PER_MINUTE - get_second()) +
           (MINUTES_PER_HOUR - get_minute()) * SECONDS_PER_MINUTE +
           (HOURS_PER_DAY - get_hour()) * SECONDS_PER_MINUTE * MINUTES_PER_HOUR;
}
