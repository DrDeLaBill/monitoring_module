/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef INC_PRESSURE_SENSOR_H_
#define INC_PRESSURE_SENSOR_H_


#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#include "utils.h"


#define PRESS_MEASURE_COUNT 30


typedef struct _press_measure_t {
	bool        measure_ready;
	uint32_t    value;
	uint8_t     measure_values_idx;
	uint16_t    measure_values[PRESS_MEASURE_COUNT];
	dio_timer_t wait_timer;
} press_measure_t;


void pressure_sensor_proccess();
uint32_t get_press();


#endif /* INC_PRESSURE_SENSOR_H_ */
