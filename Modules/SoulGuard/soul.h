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

	WAIT_LOAD,
	MODBUS_FAULT,
	PUMP_FAULT,
	RTC_FAULT,
	NEED_INIT_RECORD_TMP,
	NEED_SAVE_RECORD,

	/* Device statuses end */
	STATUSES_END,

	/* Device errors start */
	ERRORS_START,

	SETTINGS_LOAD_ERROR,
	INTERNAL_ERROR,
	MEMORY_ERROR,
	POWER_ERROR,
	STACK_ERROR,
	LOAD_ERROR,
	RAM_ERROR,

	/* Device errors end */
	ERRORS_END,

	/* Paste device errors or statuses to the top */
	SOUL_STATUSES_END
} SOUL_STATUS;


typedef struct _soul_t {
	uint8_t errors[__div_up(SOUL_STATUSES_END - 1, BITS_IN_BYTE)];
} soul_t;


bool has_errors();

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
