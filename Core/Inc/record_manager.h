#ifndef INC_RECORD_MANAGER_H_
#define INC_RECORD_MANAGER_H_


#include "defines.h"


typedef enum _record_status_t {
	RECORD_OK = 0,
	RECORD_ERROR,
	RECORD_NO_LOG
} record_status_t;


#define RECORD_FIRST_ID 1


#define RECORD_TIME_ARRAY_SIZE 6
typedef struct _log_record_t {
	uint32_t id;                          // Record ID
	uint8_t time[RECORD_TIME_ARRAY_SIZE]; // Record time
	uint32_t fw_id;                       // Firmware version
	uint32_t cf_id;                       // Configuration version
	int32_t level;                        // Liquid level
	uint32_t press_1;                     // First sensor pressure
//	uint32_t press_2;                     // Second sensor pressure
} log_record_t;


extern log_record_t cur_log_record;


record_status_t next_record_load();
record_status_t record_save();
record_status_t get_new_id();

#endif /* INC_RECORD_MANAGER_H_ */
