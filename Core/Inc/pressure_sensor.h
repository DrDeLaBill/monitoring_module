/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef INC_PRESSURE_SENSOR_H_
#define INC_PRESSURE_SENSOR_H_


#ifdef __cplusplus
extern "C" {
#endif


#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#include "gutils.h"


#define PRESS_MEASURE_COUNT 30


typedef struct _press_measure_t {
	bool             measure_ready;
	uint16_t         value;
	uint8_t          measure_values_idx;
	uint16_t         measure_values[PRESS_MEASURE_COUNT];
	util_old_timer_t wait_timer;
} press_measure_t;


void pressure_sensor_proccess();
uint16_t get_press();


#ifdef __cplusplus
}
#endif


#endif /* INC_PRESSURE_SENSOR_H_ */
