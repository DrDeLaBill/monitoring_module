/*
 * pump.h
 *
 *  Created on: Oct 19, 2022
 *      Author: georgy
 */

#ifndef INC_PUMP_H_
#define INC_PUMP_H_

#include "stm32f1xx_hal.h"

void pump_init();
void pump_proccess();
void pump_show_work();
void pump_update_speed(uint32_t speed);
void pump_update_power(uint8_t enable_state);

#endif /* INC_PUMP_H_ */
