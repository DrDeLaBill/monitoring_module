#ifndef INC_SETTINGS_MANAGER_H_
#define INC_SETTINGS_MANAGER_H_


#include "defines.h"


typedef enum _settings_status_t {
	SETTINGS_OK = 0,
	SETTINGS_ERROR
} settings_status_t;


// TODO: server id get from server
// TODO: pump speed must be recalculate by liquid level, after receive from server
typedef struct _module_settings_t {
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
	uint32_t milliliters_per_day;
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
} module_settings_t;


extern module_settings_t module_settings;

extern const char default_server_url[CHAR_SETIINGS_SIZE];
extern const char default_server_port[CHAR_SETIINGS_SIZE];


settings_status_t settings_reset();
settings_status_t settings_load();
settings_status_t settings_save();

void show_settings();


#endif /* INC_SETTINGS_MANAGER_H_ */
