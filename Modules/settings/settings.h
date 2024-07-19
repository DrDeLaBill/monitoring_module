/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "main.h"


/*
 * Device types:
 * 0x0001 - Dispenser
 * 0x0002 - Gas station
 * 0x0003 - Logger
 * 0x0004 - B.O.B.A.
 * 0x0005 - Calibrate station
 * 0x0006 - Dispenser-mini
 */
#define DEVICE_TYPE           ((uint16_t)0x0001)
#define DEFAULT_ID            ((uint8_t)0x01)
#define CHAR_SETIINGS_SIZE    (30)
#define DEVICE_ID_SIZE        ((uint8_t)16)


typedef enum _SettingsStatus {
    SETTINGS_OK = 0,
    SETTINGS__ERROR
} SettingsStatus;


typedef enum _LimitType {
	LIMIT_DAY   = 0x01,
	LIMIT_MONTH = 0x02
} LimitType;


// TODO: server id get from server
// TODO: pump speed must be recalculate by liquid level, after receive from server
typedef struct __attribute__((packed)) _settings_t  {
	uint32_t id;
	char server_url[CHAR_SETIINGS_SIZE];
	char server_port[CHAR_SETIINGS_SIZE];
	// Configuration version
	uint32_t cf_id;
	// Enable pump
	uint8_t pump_enabled;
	// Measure delay in milliseconds
	uint32_t sleep_time;
	// Current server log ID
	uint32_t server_log_id;
	// Liters ADC value when liquid tank can be considered full
	uint32_t tank_ADC_min;
	// Liters ADC value when liquid tank can be considered empty
	uint32_t tank_ADC_max;
	// Liters value when liquid tank can be considered full
	uint32_t tank_liters_max;
	// Liters value when liquid tank can be considered empty
	uint32_t tank_liters_min;
	// Target milliliters per day for pump
	uint32_t pump_target;
	// Pump speed: milliliters per hour
	uint32_t pump_speed;
	// Current pump work sec
	uint32_t pump_work_sec;
	// Current day pump downtime sec
	uint32_t pump_downtime_sec;
	// Pump work sec for current day
	uint32_t pump_work_day_sec;
	// Current log day
	uint8_t pump_log_date;
	// First start flag
	uint8_t is_first_start;
} settings_t;


extern settings_t settings;


typedef struct _settings_info_t {
	bool settings_initialized;
	bool settings_saved;
	bool settings_updated;
	bool saved_new_data;
} settings_info_t;


/* copy settings to the target */
settings_t* settings_get();
/* copy settings from the target */
void settings_set(settings_t* other);
/* reset current settings to default */
void settings_reset(settings_t* other);

uint32_t settings_size();

bool settings_check(settings_t* other);

void settings_show();

bool is_settings_saved();
bool is_settings_updated();
bool is_settings_initialized();
bool is_new_data_saved();

void set_settings_initialized();
void set_settings_save_status(bool state);
void set_settings_update_status(bool state);
void set_new_data_saved(bool state);

void settings_set_id(uint32_t cf_id);
void settings_set_cf_id(uint32_t id);

SettingsStatus settings_get_card_idx(uint32_t card, uint16_t* idx);
void settings_check_residues();



static const char defaultUrl[CHAR_SETIINGS_SIZE]  = "urv.iot.turtton.ru";
static const char defaultPort[CHAR_SETIINGS_SIZE] = "80";


#ifdef __cplusplus
}
#endif


#endif
