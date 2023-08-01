#include "record_manager.h"

#include <string.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

#include "main.h"
#include "sim_module.h"
#include "storage_data_manager.h"
#include "settings_manager.h"
#include "logger.h"
#include "utils.h"


#define CLUSTERS_MIN 10


const char* RECORD_TAG = "RCRD";;


log_record_t log_record = {0};
log_ids_cache_t log_ids_cache = {
    .ids_cache         = {{0}},
    .first_record_addr = 0xFFFFFFFF,
    .is_need_to_scan   = true
};


record_status_t _record_get_next_cache_record_address(uint32_t* addr, uint16_t* record_num);
record_status_t _record_get_next_cache_available_address(uint32_t* addr, uint16_t* record_num);
void            _record_get_next_cache_id(uint32_t* new_id);
record_status_t _record_cache_scan_storage_records();


record_status_t next_record_load() {
#if RECORD_DEBUG
    LOG_DEBUG(RECORD_TAG, "reccord load: begin\n");
#endif
    uint32_t needed_addr = 0;
    uint16_t needed_record_num = 0;

    record_status_t status = RECORD_OK;
    if (log_ids_cache.is_need_to_scan) {
        status = _record_cache_scan_storage_records();
    }

    if (status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "reccord load: error scan\n");
#endif
        return status;
    }

    status = _record_get_next_cache_record_address(&needed_addr, &needed_record_num);
    if (status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "reccord load: error get available address by cache\n");
#endif
        return status;
    }

    if (!needed_addr) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "reccord load: storage load - error=%i, needed_addr=%lu\n", status, needed_addr);
#endif
        return STORAGE_ERROR;
    }

    log_record_clust_t buff;
    memset((uint8_t*)&buff, 0 ,sizeof(buff));
    status = storage_load(needed_addr, (uint8_t*)&buff, sizeof(buff));
    if (status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "next reccord load:  error=%i\n", status);
#endif
        return RECORD_ERROR;
    }

    memcpy((uint8_t*)&log_record, (uint8_t*)&buff.records[needed_record_num], sizeof(log_record));

#if RECORD_DEBUG
    LOG_DEBUG(RECORD_TAG, "reccord load: end, loaded from page %lu (address=%lu)\n", needed_addr / STORAGE_PAGE_SIZE, needed_addr);
#endif

    return RECORD_OK;
}


record_status_t record_save() {
#if RECORD_DEBUG
    LOG_DEBUG(RECORD_TAG, "reccord save: begin\n");
#endif
    uint32_t needed_addr = 0;
    uint16_t needed_record_num = 0;

    record_status_t status = RECORD_OK;
    if (log_ids_cache.is_need_to_scan) {
        status = _record_cache_scan_storage_records();
    }

    if (status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "reccord save: error scan\n");
#endif
        return status;
    }

    status = _record_get_next_cache_available_address(&needed_addr, &needed_record_num);
    if (status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "reccord save: error get available address by cache\n");
#endif
        return status;
    }

    if (!needed_addr) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "reccord save: storage load - error=%i, needed_addr=%lu\n", status, needed_addr);
#endif
        return STORAGE_ERROR;
    }

    log_record_clust_t buff;
    memset((uint8_t*)&buff, 0 ,sizeof(buff));
    status = storage_load(needed_addr, (uint8_t*)&buff, sizeof(buff));
	if (status != RECORD_OK) {
#if RECORD_DEBUG
		LOG_DEBUG(RECORD_TAG, "reccord save: storage load - error=%i, needed_addr=%lu\n", status, needed_addr);
#endif
		return status;
	}

	memcpy((uint8_t*)&buff.records[needed_record_num], (uint8_t*)&log_record, sizeof(log_record_t));

    status = storage_save(needed_addr, (uint8_t*)&buff, sizeof(buff));
    if (status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "reccord save: storage save - error=%i, needed_addr=%lu\n", status, needed_addr);
#endif
        return status;
    }

#if RECORD_DEBUG
    LOG_DEBUG(RECORD_TAG, "reccord save: end, saved on page %lu (address=%lu)\n", needed_addr / STORAGE_PAGE_SIZE, needed_addr);
#endif

    log_ids_cache.ids_cache[needed_addr / STORAGE_PAGE_SIZE][needed_record_num] = log_record.id;

    return STORAGE_OK;
}

record_status_t get_new_id(uint32_t* new_id)
{
#if RECORD_DEBUG
    LOG_DEBUG(RECORD_TAG, "get new id: begin\n");
#endif
    *new_id = RECORD_FIRST_ID;

    record_status_t status = RECORD_OK;
    if (log_ids_cache.is_need_to_scan) {
        status = _record_cache_scan_storage_records();
    }

    if (status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "get new id: error scan\n");
#endif
        return status;
    }

    _record_get_next_cache_id(new_id);

#if RECORD_DEBUG
    LOG_DEBUG(RECORD_TAG, "get new id: end, got max id=%lu\n", *new_id);
#endif

    return RECORD_OK;
}

record_status_t _record_cache_scan_storage_records()
{
    LOG_MESSAGE(RECORD_TAG, "Scanning storage. Please wait.\n");
    uint32_t scan_start_time = HAL_GetTick();
    memset(log_ids_cache.ids_cache, 0, sizeof(log_ids_cache.ids_cache));

#if RECORD_DEBUG
    LOG_DEBUG(RECORD_TAG, "scan storage: begin\n");
#endif

    uint32_t first_addr = 0;
    uint32_t prev_addr = 0;
    storage_status_t status = storage_get_first_available_addr(&first_addr);
    if (status != STORAGE_OK) {
#if RECORD_DEBUG
        LOG_DEBUG(RECORD_TAG, "scan storage: error get prev address - status:=%d, address:=%lu\n", status, prev_addr);
#endif
        return RECORD_ERROR;
    }
    prev_addr = first_addr;

    uint32_t max_id = 0;
    uint32_t next_addr = 0;
    log_record_clust_t buff;
    memset((uint8_t*)&buff, 0 ,sizeof(buff));

    uint8_t last_debug_percent = 0, new_debug_percent = 0;
	uint8_t msg[] = "[          ]";
    while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
    	if (last_debug_percent != new_debug_percent) {
    		LOG_MESSAGE(RECORD_TAG, "%s\n", msg);
    		last_debug_percent = new_debug_percent;
    	}

        status = storage_get_next_available_addr(prev_addr, &next_addr);
        if (status == STORAGE_ERROR_OUT_OF_MEMORY) {
            continue;
        }
        if (status != STORAGE_OK) {
#if RECORD_DEBUG
            LOG_DEBUG(RECORD_TAG, "scan storage: error get next address - status:=%d, prev-address:=%lu, next-address:=%lu\n", status, prev_addr, next_addr);
#endif
            return RECORD_ERROR;
        }

        if (prev_addr == first_addr) {
            log_ids_cache.first_record_addr = next_addr;
        }

        status = storage_load(next_addr, (uint8_t*)&buff, sizeof(buff));
        if (status == STORAGE_ERROR_BLOCKED ||
            status == STORAGE_ERROR_BITS ||
            status == STORAGE_ERROR_BADACODE ||
            status == STORAGE_ERROR_VER ||
            status == STORAGE_ERROR_APPOINTMENT
        ) {
            goto do_next_addr;
        }
        if (status != STORAGE_OK) {
#if RECORD_DEBUG
            LOG_DEBUG(RECORD_TAG, "scan storage: read record error=%i, address=%lu\n", status, next_addr);
#endif
            return RECORD_ERROR;
        }

        for (uint16_t i = 0; i < RECORDS_CLUST_SIZE; i++) {
        	log_ids_cache.ids_cache[next_addr / STORAGE_PAGE_SIZE][i] = buff.records[i].id;

            if (max_id < buff.records[i].id) {
                max_id = buff.records[i].id;
            }
        }

do_next_addr:
        prev_addr = next_addr;
        new_debug_percent = ((prev_addr * 10) / STORAGE_SIZE) * 10;
		memset(msg + 1, '=', (new_debug_percent / 10) % sizeof(msg));
    }

    if (status == STORAGE_ERROR_OUT_OF_MEMORY) {
        status = STORAGE_OK;
    }

    if (status == STORAGE_OK) {
		memset(msg + 1, '=', 10);
		LOG_MESSAGE(RECORD_TAG, "%s\n", msg);
        log_ids_cache.is_need_to_scan = false;
    } else {
    	LOG_MESSAGE(RECORD_TAG, "scan storage: fail, error=%02x\n", status);
    }

    LOG_MESSAGE(RECORD_TAG, "Scan storage: end. Time=%lu ms.\n", __abs_dif(HAL_GetTick(), scan_start_time));

    return status;
}

void _record_get_next_cache_id(uint32_t* new_id)
{
    *new_id = RECORD_FIRST_ID;
#if RECORD_DEBUG
    uint32_t addr = 0;
#endif
    for (uint32_t i = 0; i < STORAGE_PAGES_COUNT; i++) {
        for (uint16_t j = 0; j < RECORDS_CLUST_SIZE; j++) {
			if (log_ids_cache.ids_cache[i][j] > *new_id) {
				*new_id = log_ids_cache.ids_cache[i][j];
#if RECORD_DEBUG
				addr = i * STORAGE_PAGE_SIZE;
#endif
			}
        }
    }
    *new_id += 1;
#if RECORD_DEBUG
    LOG_DEBUG(RECORD_TAG, "get new id: end, got max id from cache page %lu, new id=%lu\n", addr, *new_id);
#endif
}

record_status_t _record_get_next_cache_record_address(uint32_t* addr, uint16_t* record_num)
{
    if (!log_ids_cache.first_record_addr) {
        return RECORD_ERROR;
    }
    uint32_t last_id = module_settings.server_log_id;
    for (uint32_t i = log_ids_cache.first_record_addr / STORAGE_PAGE_SIZE; i < STORAGE_PAGES_COUNT; i++) {
        for (uint16_t j = 0; j < RECORDS_CLUST_SIZE; j++) {
			if (log_ids_cache.ids_cache[i][j] <= module_settings.server_log_id) {
				continue;
			}
			if (!last_id || last_id > log_ids_cache.ids_cache[i][j]) {
				last_id = log_ids_cache.ids_cache[i][j];
				*addr = i * STORAGE_PAGE_SIZE;
				*record_num = j;
			}
        }
    }
    if (last_id == module_settings.server_log_id) {
        return RECORD_NO_LOG;
    }
    return RECORD_OK;
}

record_status_t _record_get_next_cache_available_address(uint32_t* addr, uint16_t* record_num)
{
    if (!log_ids_cache.first_record_addr) {
        return RECORD_ERROR;
    }
    uint32_t min_id = 0xFFFFFFFF;
    uint32_t min_id_addr = 0;
    uint16_t min_id_num = 0;
    for (uint32_t i = log_ids_cache.first_record_addr / STORAGE_PAGE_SIZE; i < STORAGE_PAGES_COUNT; i++) {
    	for (uint16_t j = 0; j < RECORDS_CLUST_SIZE; j++) {
			if (!log_ids_cache.ids_cache[i][j]) {
				*addr = i * STORAGE_PAGE_SIZE;
				*record_num = j;
				return RECORD_OK;
			}
			if (min_id > log_ids_cache.ids_cache[i][j]) {
				min_id = log_ids_cache.ids_cache[i][j];
				min_id_addr = i * STORAGE_PAGE_SIZE;
				min_id_num = j;
			}
    	}
    }
    if (!min_id_addr) {
        return RECORD_ERROR;
    }
    *addr = min_id_addr;
	*record_num = min_id_num;
    return RECORD_OK;
}
