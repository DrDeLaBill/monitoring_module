/*
 * pump.h
 *
 *  Created on: Oct 19, 2022
 *      Author: DrDeLaBill
 */

#ifndef INC_PUMP_H_
#define INC_PUMP_H_

#include <stdint.h>
#include <stdbool.h>

#include "utils.h"


typedef struct _pump_state_t {
	void        (*state_action) (void);
	bool        enabled;
	uint32_t    start_time;
	uint32_t    needed_work_time;
	dio_timer_t wait_timer;
} pump_state_t;


void pump_init();
void pump_proccess();
void pump_update_speed(uint32_t speed);
void pump_update_enable_state(bool enabled);
void pump_reset_work_state();
void pump_show_status();
void pump_clear_log();

#endif /* INC_PUMP_H_ */
