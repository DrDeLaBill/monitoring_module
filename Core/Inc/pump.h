/*
 * pump.h
 *
 *  Created on: Oct 19, 2022
 *      Author: georgy
 */

#ifndef INC_PUMP_H_
#define INC_PUMP_H_

#include "stm32f1xx_hal.h"
#include <stdbool.h>

#include "utils.h"


enum {
	PUMP_OFF = 0,
	PUMP_WORK,
	PUMP_WAIT
};


typedef struct _pump_state {
	void (*state_action) (void);
	uint32_t start_time;
	uint32_t needed_work_time;
	dio_timer_t work_timer;
	uint8_t state;
	uint16_t overage_work_time;
} PumpState;


void pump_init();
void pump_proccess();
void pump_update_speed(uint32_t speed);
void pump_update_work();
void pump_update_power(bool enabled);
void pump_show_work();
void clear_pump_log();

#endif /* INC_PUMP_H_ */
