/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef __SOUL_H
#define __SOUL_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "hal_defs.h"


typedef enum _SOUK_STATUS {
	/* Device statuses start */
	STATUSES_START,

	MODBUS_FAULT,
	PUMP_FAULT,
	RTC_FAULT,

	/* Device statuses end */
	STATUSES_END,

	/* Device errors start */
	ERRORS_START,

	INTERNAL_ERROR,
	SETTINGS_ERROR,
	MEMORY_ERROR,
	STACK_ERROR,
	RAM_ERROR,

	/* Device errors end */
	ERRORS_END,

	/* Paste device errors or statuses to the top */
	SOUL_STATUSES_END
} SOUL_STATUS;


typedef struct _soul_t {
	uint8_t errors[__div_up(SOUL_STATUSES_END - 1, BITS_IN_BYTE)];
} soul_t;


bool is_error(SOUL_STATUS error);
void set_error(SOUL_STATUS error);
void reset_error(SOUL_STATUS error);

bool is_status(SOUL_STATUS status);
void set_status(SOUL_STATUS status);
void reset_status(SOUL_STATUS status);


#ifdef __cplusplus
}
#endif


#endif
