/*
 * pump.h
 *
 *  Created on: Oct 19, 2022
 *      Author: georgy
 */

#ifndef INC_PUMP_H_
#define INC_PUMP_H_

#include "stm32f1xx_hal.h"

void pump_init(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void pump_proccess();

#endif /* INC_PUMP_H_ */
