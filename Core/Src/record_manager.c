#include "record_manager.h"

#include <string.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

#include "sim_module.h"
#include "storage_data_manager.h"
#include "settings_manager.h"
#include "logger.h"
#include "utils.h"


#define CLUSTERS_MIN 10


uint32_t _get_free_space();


const char* RECORD_TAG = "RCRD";;


bool new_record_available = true;
log_record_t cur_log_record;


record_status_t next_record_load() {
	if (!new_record_available) {
		return RECORD_NO_LOG;
	}

	uint32_t first_addr = 0;
	uint32_t prev_addr = 0;
	storage_status_t status = storage_get_first_available_addr(&first_addr);
	if (status != STORAGE_OK) {
		LOG_DEBUG(RECORD_TAG,  " next reccord load: error get prev address - status:=%d, address:=%lu\n", status, prev_addr);
		return RECORD_ERROR;
	}
	prev_addr = first_addr;


	uint32_t last_id = 0;
	uint32_t next_addr = 0;
	uint32_t needed_addr = 0;
	log_record_t buff;
	memset((uint8_t*)&buff, 0 ,sizeof(buff));
	while (status != STORAGE_EOM) {
		status = storage_get_next_available_addr(prev_addr, &next_addr);
		if (status == STORAGE_EOM) {
			continue;
		}
		if (status != STORAGE_OK) {
			LOG_DEBUG(RECORD_TAG,  " next reccord load: error get next address - status:=%d, prev-address:=%lu, next-address:=%lu\n", status, prev_addr, next_addr);
			return RECORD_ERROR;
		}

		status = storage_load(next_addr, (uint8_t*)&buff, sizeof(buff));
		if (status == STORAGE_ERROR_ADDR) {
			prev_addr = needed_addr;
			continue;
		}
		if (status != STORAGE_OK) {
			LOG_DEBUG(RECORD_TAG, " read record error=%i, address=%lu\n", status, next_addr);
			return RECORD_ERROR;
		}

		if (buff.id <= module_settings.server_log_id) {
			prev_addr = next_addr;
			continue;
		}

		if (!last_id) {
			last_id = buff.id;
			needed_addr = next_addr;
		}

		if (__abs_dif(module_settings.server_log_id, last_id) > __abs_dif(module_settings.server_log_id, buff.id)) {
			last_id = buff.id;
			needed_addr = next_addr;
		}

		prev_addr = next_addr;
	}

	if (status != STORAGE_OK) {
		LOG_DEBUG(RECORD_TAG,  " next reccord load: not found - status=%d, address=%lu, log_id=%lu\n", status, needed_addr, last_id);
		return RECORD_ERROR;
	}

	status = storage_load(needed_addr, (uint8_t*)&buff, sizeof(buff));
	if (status != STORAGE_OK) {
		LOG_DEBUG(RECORD_TAG, " next reccord load:  error=%i\n", status);
		return RECORD_ERROR;
	}

	memcpy((uint8_t*)&cur_log_record, (uint8_t*)&buff, sizeof(cur_log_record));

	return RECORD_OK;
}


record_status_t record_save() {
	new_record_available = true;

	uint32_t first_addr = 0;
	storage_status_t status = storage_get_first_available_addr(&first_addr);
	if (status != STORAGE_OK) {
		LOG_DEBUG(RECORD_TAG, " reccord save: get first addr - error=%i\n", status);
		return status;
	}

	uint32_t next_addr = 0;
	log_record_t buff;
	memset((uint8_t*)&buff, 0 ,sizeof(buff));
	uint32_t min_id = 0;
	uint32_t min_id_addr = 0;
	while (status != STORAGE_EOM) {
		status = storage_get_next_available_addr(first_addr, &next_addr);
		if (status == STORAGE_EOM) {
			break;
		}
		if (status != STORAGE_OK) {
			LOG_DEBUG(RECORD_TAG, " reccord save: get next addr - error=%i, prev_addr=%lu, next_addr=%lu\n", status, first_addr, next_addr);
			return status;
		}

		status = storage_load(next_addr, (uint8_t*)&buff, sizeof(buff));
		if (status != STORAGE_OK) {
			break;
		}

		if (!min_id) {
			min_id = buff.id;
			min_id_addr = next_addr;
		}
		first_addr = next_addr;
	}

	if (status == STORAGE_EOM) {
		next_addr = min_id_addr;
	}

	if (!next_addr) {
		LOG_DEBUG(RECORD_TAG, " reccord save: storage load - error=%i, prev_addr=%lu, next_addr=%lu\n", status, first_addr, next_addr);
		return STORAGE_ERROR;
	}

	status = storage_save(next_addr, (uint8_t*)&cur_log_record, sizeof(cur_log_record));
	if (status != STORAGE_OK) {
		LOG_DEBUG(RECORD_TAG, " reccord save: storage save - error=%i, prev_addr=%lu, next_addr=%lu\n", status, first_addr, next_addr);
		return status;
	}

	return STORAGE_OK;
}

record_status_t get_new_id(uint32_t* new_id)
{
	uint32_t prev_addr = 0;
	storage_status_t status = storage_get_first_available_addr(&prev_addr);
	if (status != STORAGE_OK) {
		LOG_DEBUG(RECORD_TAG,  " get new id: error get prev address - status:=%d, address:=%lu\n", status, prev_addr);
		return RECORD_ERROR;
	}

	uint32_t max_id = 0;
	uint32_t next_addr = 0;
	log_record_t buff;
	memset((uint8_t*)&buff, 0 ,sizeof(buff));
	while (status != STORAGE_EOM) {
		status = storage_get_next_available_addr(prev_addr, &next_addr);
		if (status == STORAGE_EOM) {
			continue;
		}
		if (status != STORAGE_OK) {
			LOG_DEBUG(RECORD_TAG,  " get new id: error get next address - status:=%d, prev-address:=%lu, next-address:=%lu\n", status, prev_addr, next_addr);
			return RECORD_ERROR;
		}

		status = storage_load(next_addr, (uint8_t*)&buff, sizeof(buff));
		if (status == STORAGE_ERROR_ADDR) {
			prev_addr = next_addr;
			continue;
		}
		if (status == STORAGE_ERROR_BITS) {
			prev_addr = next_addr;
			continue;
		}
		if (status != STORAGE_OK) {
			LOG_DEBUG(RECORD_TAG, " get new id: read record error=%i, address=%lu\n", status, next_addr);
			return RECORD_ERROR;
		}

		if (max_id < buff.id) {
			max_id = buff.id;
		}

		prev_addr = next_addr;
	}

	*new_id = max_id + 1;

	return RECORD_OK;
}
