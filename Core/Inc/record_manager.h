#ifndef INC_RECORD_MANAGER_H_
#define INC_RECORD_MANAGER_H_


#include <stdbool.h>

#include "defines.h"
#include "storage_data_manager.h"


#define RECORD_DEBUG (false)


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
	uint32_t cf_id;                       // Configuration version
	int32_t level;                        // Liquid level
	uint32_t press_1;                     // First pressure sensor
//	uint32_t press_2;                     // Second pressure sensor
	uint32_t pump_wok_time;               // Log pump downtime sec
	uint32_t pump_downtime;               // Log pump work sec
} log_record_t;

#define RECORDS_CLUST_SIZE  ((STORAGE_PAYLOAD_SIZE - sizeof(uint8_t)) / sizeof(struct _log_record_t))
#define RECORDS_CLUST_MAGIC (sizeof(struct _log_record_t))
typedef struct _log_record_clust_t {
	uint8_t record_magic;
	log_record_t records[RECORDS_CLUST_SIZE];
}log_record_clust_t;

typedef struct _log_ids_cache_t {
	bool is_need_to_scan;
	uint32_t cur_scan_address;
	uint32_t first_record_addr;
	uint32_t ids_cache[STORAGE_PAGES_COUNT][RECORDS_CLUST_SIZE];
} log_ids_cache_t;


extern log_record_t log_record;


record_status_t next_record_load();
record_status_t record_save();
record_status_t record_get_new_id();
record_status_t record_delete_record(uint32_t id);
void            record_cache_records_proccess();


#endif /* INC_RECORD_MANAGER_H_ */
