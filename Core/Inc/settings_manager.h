/*
 * settings_manager.h
 *
 *  Created on: May 27, 2022
 *      Author: gauss
 */

#ifndef INC_SETTINGS_MANAGER_H_
#define INC_SETTINGS_MANAGER_H_

#include "defines.h"
#include "internal_storage.h"


typedef enum _settings_status_t {
	SETTINGS_OK = 0,
	SETTINGS_ERROR
} settings_status_t;


typedef struct _settings_cal_point_t {
	uint8_t valid;
	int16_t temp_whole;
	int16_t temp_100th;
	int16_t adc_value;
} settings_cal_point_t;

// TODO: убрать
//typedef struct _settings_cal_line_t {
//	uint16_t channel;
//	uint16_t npoints;
//	settings_cal_point_t points[MEASURE_MAX_CAL_POINTS];
//} settings_cal_line_t;


#define SETTINGS_SD_PAYLOAD_MAGIC ((uint32_t)(0xBADAC0DE))
#define SETTINGS_SD_PAYLOAD_VERSION (1)


typedef char channel_name_t[4];


#define SETTINGS_SD_MAX_PAYLOAD_SIZE 128


typedef struct _module_settings {
	uint32_t id;
	uint8_t server_url[CHAR_SETIINGS_SIZE];
	uint8_t server_port[CHAR_SETIINGS_SIZE];
	// Liters ADC value when liquid tank can be considered full
	uint16_t tank_ADC_min;
	// Liters ADC value when liquid tank can be considered empty
	uint16_t tank_ADC_max;
	// Liters value when liquid tank can be considered full
	uint16_t tank_liters_max;
	// Liters value when liquid tank can be considered empty
	uint16_t tank_liters_min;
	// Target milliliters per day for pump
	uint32_t milliliters_per_day;
	// Pump speed: milliliters per hour
	uint32_t pump_speed;
	// External clock state
	uint8_t clock_initialized;
	// Measure delay in milliseconds
	uint32_t sleeping_time;
} ModuleSettings;


typedef union _settings_sd_payload_t {
	struct __attribute__((packed)) {
		struct _sd_payload_header_t header;
		uint8_t bits[SD_PAYLOAD_BITS_SIZE(SETTINGS_SD_MAX_PAYLOAD_SIZE)];
		uint16_t crc;
	};
	struct __attribute__((packed)) {
		struct _sd_payload_header_t header;
		//
		ModuleSettings payload_settings;
	} v1;
} settings_sd_payload_t;


typedef void (*settings_default_cb_t)(settings_sd_payload_t* payload);
typedef void (*settings_save_cb_t)(settings_sd_payload_t* payload);
typedef void (*settings_load_cb_t)(const settings_sd_payload_t* payload);


typedef struct _settings_tag_t {
	settings_default_cb_t default_cb; // fill-in default settings
	settings_save_cb_t save_cb; 	  // create serialized representation
	settings_load_cb_t load_cb; 	  // get data from serialized representation
} settings_tag_t;


extern settings_tag_t* settings_cbs[];
extern uint8_t settings_load_ok;


void settings_reset();
settings_status_t settings_load();
settings_status_t settings_save();




#endif /* INC_SETTINGS_MANAGER_H_ */
