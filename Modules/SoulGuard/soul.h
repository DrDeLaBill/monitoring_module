/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _SOUL_H_
#define _SOUL_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "gutils.h"
#include "hal_defs.h"


typedef enum _SOUK_STATUS {
	/* Device statuses start */
	STATUSES_START = 0,

	LOADING,
	WORKING,
	RCC_FAULT,
	MEMORY_READ_FAULT,
	MEMORY_WRITE_FAULT,
	NEED_MEASURE,
	NEED_STANDBY,
	SETTINGS_INITIALIZED,
	NEED_LOAD_SETTINGS,
	NEED_SAVE_SETTINGS,
	NO_SENSOR,
	MANUAL_NEED_VALVE_UP,
	MANUAL_NEED_VALVE_DOWN,
	AUTO_NEED_VALVE_UP,
	AUTO_NEED_VALVE_DOWN,
	MODBUS_FAULT,
	PUMP_FAULT,
	RTC_FAULT,
	CAN_FAULT,
	NO_BIGSKI,


	RECORD_UPDATED,
	NEED_INIT_RECORD_TMP,
	NEED_SAVE_RECORD_TMP,
	NEED_SAVE_FINAL_RECORD,
	NEED_SERVICE_BACK,
	NEED_SERVICE_SAVE,
	NEED_SERVICE_UPDATE,
	NEED_REGISTRATE_1WIRE,
	NEED_ENABLE_SENSORS,

	/* Device statuses end */
	STATUSES_END,

	/* Device errors start */
	ERRORS_START,

	MCU_ERROR,
	RCC_ERROR,
	RTC_ERROR,
	POWER_ERROR,
	EXPECTED_MEMORY_ERROR,
	MEMORY_ERROR,
	MEMORY_INIT_ERROR,
	STACK_ERROR,
	RAM_ERROR,
	SD_CARD_ERROR,
	USB_ERROR,
	SETTINGS_LOAD_ERROR,
	APP_MODE_ERROR,
	PUMP_ERROR,
	VALVE_ERROR,
	FATFS_ERROR,
	LOAD_ERROR,

	NON_MASKABLE_INTERRUPT,
	HARD_FAULT,
	MEM_MANAGE,
	BUS_FAULT,
	USAGE_FAULT,

	ASSERT_ERROR,
	ERROR_HANDLER_CALLED,
	INTERNAL_ERROR,

	/* Device errors end */
	ERRORS_END,

	/* Paste device errors or statuses to the top */
	SOUL_STATUSES_END
} SOUL_STATUS;


typedef struct _soul_t {
	unsigned last_err;
	uint8_t statuses[__div_up(SOUL_STATUSES_END - 1, BITS_IN_BYTE)];
} soul_t;


unsigned get_last_error();
void set_last_error(SOUL_STATUS error);

bool has_errors();

bool is_error(SOUL_STATUS error);
void set_error(SOUL_STATUS error);
void reset_error(SOUL_STATUS error);
unsigned get_first_error();

bool is_status(SOUL_STATUS status);
void set_status(SOUL_STATUS status);
void reset_status(SOUL_STATUS status);


#ifdef __cplusplus
}
#endif


#endif
