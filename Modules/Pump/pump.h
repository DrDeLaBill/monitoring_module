/*
 * pump.h
 *
 *  Created on: Oct 19, 2022
 *      Author: DrDeLaBill
 */

#ifndef INC_PUMP_H_
#define INC_PUMP_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "gutils.h"


#ifdef DEBUG
#   define PUMP_BEDUG (1)
#endif


typedef struct _pump_state_t {
	void             (*state_action) (void);
	bool             enabled;
	uint32_t         start_time;
	uint32_t         needed_work_time;
	util_old_timer_t wait_timer;
	util_old_timer_t indication_timer;
} pump_state_t;


void pump_init();
void pump_proccess();
void pump_update_speed(uint32_t speed);
void pump_update_enable_state(bool enabled);
void pump_update_ltrmin(uint32_t ltrmin);
void pump_update_ltrmax(uint32_t ltrmax);
void pump_update_target(uint32_t target);
void pump_reset_work_state();
void pump_show_status();
void pump_clear_log();


#ifdef __cplusplus
}
#endif


#endif /* INC_PUMP_H_ */
