/*
 * data_logger.h
 *
 *  Created on: Dec 15, 2022
 *      Author: RCPark
 */

#ifndef INC_RECORD_MANAGER_H_
#define INC_RECORD_MANAGER_H_

#include "defines.h"
#include "internal_storage.h"


typedef enum _record_status_t {
	RECORD_OK = 0,
	RECORD_ERROR,
	RECORD_NO_LOG
} record_status_t;


#define RECORD_SD_PAYLOAD_MAGIC ((uint32_t)(0xBADAC0DE))
#define RECORD_SD_PAYLOAD_VERSION (1)


#define SETTINGS_SD_MAX_PAYLOAD_SIZE 512

#define FIRST_ID 1

typedef struct _log_record {
	uint32_t id;                // Record ID
	char time[CHAR_PARAM_SIZE]; // Record time
	uint32_t fw_id;             // Firmware version
	uint32_t cf_id;             // Configuration version
	float level;                // Liquid level
	float press_1;              // First sensor pressure
	float press_2;              // Second sensor pressure
} LogRecord;

typedef union _record_sd_payload_t {
	struct __attribute__((packed)) {
		struct _sd_payload_header_t header;
		uint8_t bits[SD_PAYLOAD_BITS_SIZE(SETTINGS_SD_MAX_PAYLOAD_SIZE)];
		uint16_t crc;
	};
	struct __attribute__((packed)) {
		struct _sd_payload_header_t header;
		//
		LogRecord payload_record;
	} v1;
} record_sd_payload_t;


typedef void (*record_save_cb_t)(record_sd_payload_t* payload);
typedef void (*record_load_cb_t)(const record_sd_payload_t* payload);

typedef struct _record_tag_t {
	record_save_cb_t save_cb; 	  // create serialized representation
	record_load_cb_t load_cb; 	  // get data from serialized representation
} record_tag_t;

extern record_tag_t* record_cbs[];
extern uint8_t record_load_ok;

void record_manager_begin();
record_status_t next_record_load();
record_status_t record_save();
record_status_t record_change(uint32_t old_id);
record_status_t record_remove(uint32_t id);
record_status_t record_file_remove(const char* filename);
record_status_t remove_old_records();
uint32_t get_new_id();

#endif /* INC_RECORD_MANAGER_H_ */
