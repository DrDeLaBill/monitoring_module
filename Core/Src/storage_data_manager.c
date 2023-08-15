/*
 * storage_data_manager.c
 *
 *  Created on: Jul 25, 2023
 *      Author: DrDeLaBill
 */
#include <storage_data_manager.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "eeprom_storage.h"
#include "utils.h"


const char* STORAGE_TAG = "STRG";

bool is_error_page_updated = true;
storage_errors_list_page_t storage_errors_page_list;


storage_status_t _storage_write_page(uint32_t page_addr, uint8_t* buff, uint16_t len, uint8_t appointment);
storage_status_t _storage_read_page(uint32_t page_addr, uint8_t* buff, uint16_t len, uint8_t appointment);

storage_status_t _storage_write_errors_list_page();
storage_status_t _storage_read_errors_list_page();
storage_status_t _storage_get_errors_list_page_addr(uint32_t* addr);

void _storage_inverse_payload_bits(storage_page_record_t* payload);
bool _storage_check_inverse_bits(storage_page_record_t* payload);

bool _storage_is_page_blocked(uint16_t page_num);
void _storage_set_page_blocked(uint16_t page_num, bool blocked);


storage_status_t storage_load(uint32_t addr, uint8_t* data, uint16_t len)
{
#if STORAGE_DEBUG
	LOG_DEBUG(STORAGE_TAG, "storage load: begin\n");
#endif

    storage_status_t status = _storage_read_errors_list_page();
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "storage load: unable to get errors list page\n");
#endif
        return status;
    }

    if (_storage_is_page_blocked(addr / STORAGE_PAGE_SIZE)) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "storage load: unavailable address - errors list page\n");
#endif
        return STORAGE_ERROR_BLOCKED;
    }

    status = _storage_read_page(addr, data, len, STORAGE_APPOINTMENT_COMMON);
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "storage load: unable to get available addresses\n");
#endif
        return status;
    }

#if STORAGE_DEBUG
	LOG_DEBUG(STORAGE_TAG, "storage load: OK\n");
#endif
    return STORAGE_OK;
}

storage_status_t storage_save(uint32_t addr, uint8_t* data, uint16_t len)
{
#if STORAGE_DEBUG
	LOG_DEBUG(STORAGE_TAG, "storage save: begin\n");
#endif

    storage_status_t status = _storage_read_errors_list_page();
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "storage save: unable to get errors list page\n");
#endif
        return status;
    }
    if (_storage_is_page_blocked(addr / STORAGE_PAGE_SIZE)) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "storage save: storage page blocked\n");
#endif
        return STORAGE_ERROR_BLOCKED;
    }

    status = _storage_write_page(addr, data, len, STORAGE_APPOINTMENT_COMMON);
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "storage save: storage write error\n");
#endif
        return status;
    }

#if STORAGE_DEBUG
	LOG_DEBUG(STORAGE_TAG, "storage save: OK\n");
#endif
    return STORAGE_OK;
}

storage_status_t storage_get_first_available_addr(uint32_t* addr)
{
#if STORAGE_DEBUG
	LOG_DEBUG(STORAGE_TAG, "get first available address\n");
#endif
    return storage_get_next_available_addr(STORAGE_START_ADDR, addr);
}

storage_status_t storage_get_next_available_addr(uint32_t prev_addr, uint32_t* next_addr)
{
#if STORAGE_DEBUG
	LOG_DEBUG(STORAGE_TAG, "get next of %lu address: begin\n", prev_addr);
#endif

    if (prev_addr + sizeof(storage_page_record_t) - 1 > STORAGE_SIZE) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "get next of %lu address: error - out of memory\n", prev_addr);
#endif
        return STORAGE_ERROR_OUT_OF_MEMORY;
    }

    storage_status_t status = _storage_read_errors_list_page();
	if (status != STORAGE_OK) {
#if STORAGE_DEBUG
		LOG_DEBUG(STORAGE_TAG, "get next of %lu address: error read errors list page - try to write errors list page\n", prev_addr);
#endif
		status = storage_reset_errors_list_page();
	}

	if (status != STORAGE_OK) {
#if STORAGE_DEBUG
		LOG_DEBUG(STORAGE_TAG, "get next of %lu address: unable to read errors list page\n", prev_addr);
#endif
		return STORAGE_ERROR;
	}

	uint32_t start_addr = STORAGE_START_ADDR;
	status = _storage_get_errors_list_page_addr(&start_addr);
	if (status != STORAGE_OK) {
#if STORAGE_DEBUG
		LOG_DEBUG(STORAGE_TAG, "get next of %lu address: unable to find storage start address\n", prev_addr);
#endif
		return STORAGE_ERROR;
	}

	if (prev_addr < start_addr) {
		prev_addr = start_addr;
	}

    for (uint16_t i = prev_addr / STORAGE_PAGE_SIZE + 1; i < STORAGE_PAGES_COUNT; i++) {
        if (!_storage_is_page_blocked(i)) {
            *next_addr = i * STORAGE_PAGE_SIZE;
#if STORAGE_DEBUG
            LOG_DEBUG(STORAGE_TAG, "get next of %lu address: next address found  %lu\n", prev_addr, *next_addr);
#endif
            return STORAGE_OK;
        }
    }

#if STORAGE_DEBUG
    LOG_DEBUG(STORAGE_TAG, "get next of %lu address: there is no available next address\n", prev_addr);
#endif
    return STORAGE_ERROR_OUT_OF_MEMORY;
}

storage_status_t storage_reset_errors_list_page() {
#if STORAGE_DEBUG
    LOG_DEBUG(STORAGE_TAG, "WARNING! Trying to reset errors list page\n");
#endif
    memset((uint8_t*)&storage_errors_page_list, 0, sizeof(storage_errors_page_list));
    return _storage_write_errors_list_page();
}

storage_status_t _storage_write_page(uint32_t page_addr, uint8_t* buff, uint16_t len, uint8_t appointment)
{
	storage_page_record_t payload;
    memset((uint8_t*)&payload, 0, sizeof(payload));

#if STORAGE_DEBUG
    LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): begin\n", page_addr);
#endif

    if (len > sizeof(payload.v2.payload)) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): buffer length out of payload size\n", page_addr);
#endif
        return STORAGE_ERROR_OUT_OF_MEMORY;
    }

    if (page_addr + sizeof(payload) - 1 > STORAGE_SIZE) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): unavailable page address\n", page_addr);
#endif
        return STORAGE_ERROR_OUT_OF_MEMORY;
    }

    payload.v2.header.magic = STORAGE_PAYLOAD_MAGIC;
    payload.v2.header.version = STORAGE_PAYLOAD_VERSION;
    payload.v2.header.appointment = appointment;

    memcpy(payload.v2.payload, buff, len);

    payload.v2.crc = util_get_crc16((uint8_t*)&payload.v2, sizeof(payload) - sizeof(payload.v2.crc) - sizeof(payload.v2.invers_bits));

    _storage_inverse_payload_bits(&payload);

    storage_status_t storage_status = STORAGE_OK;
    eeprom_status_t eeprom_status = eeprom_write(page_addr, (uint8_t*)&payload, sizeof(payload));
    if (payload.v2.header.appointment == STORAGE_APPOINTMENT_COMMON && eeprom_status == EEPROM_ERROR) {
        _storage_set_page_blocked(page_addr / STORAGE_PAGE_SIZE, true);
        storage_status = _storage_write_errors_list_page();
    }
    if (eeprom_status == EEPROM_ERROR_BUSY) {
#if STORAGE_DEBUG
		LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): eeprom write error - eeprom busy\n", page_addr);
#endif
		return STORAGE_ERROR_BUSY;
	}
    if (eeprom_status != EEPROM_OK) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): eeprom write error\n", page_addr);
#endif
        return STORAGE_ERROR;
    }
    if (storage_status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): update errors list page error\n", page_addr);
#endif
        return STORAGE_ERROR;
    }

    memset((uint8_t*)&payload, 0, sizeof(payload));
    eeprom_status = eeprom_read(page_addr, (uint8_t*)&payload, sizeof(payload));
    if (eeprom_status == EEPROM_ERROR_BUSY) {
#if STORAGE_DEBUG
		LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): eeprom read error - eeprom busy\n", page_addr);
#endif
		return STORAGE_ERROR_BUSY;
	}
    if (eeprom_status != EEPROM_OK) {
#if STORAGE_DEBUG
		LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): error read record\n", page_addr);
#endif
		return STORAGE_ERROR;
	}
    if (!_storage_check_inverse_bits(&payload)) {
#if STORAGE_DEBUG
		LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): bad inverse bits\n", page_addr);
#endif
		return STORAGE_ERROR_BITS;
	}

#if STORAGE_DEBUG
    LOG_DEBUG(STORAGE_TAG, "write page (address=%lu): OK\n", page_addr);
#endif

#if STORAGE_DEBUG
	util_debug_hex_dump(STORAGE_TAG, (uint8_t*)&payload, sizeof(payload));
#endif
    return STORAGE_OK;
}

storage_status_t _storage_read_page(uint32_t page_addr, uint8_t* buff, uint16_t len, uint8_t appointment)
{
	storage_page_record_t payload;
    memset((uint8_t*)&payload, 0, sizeof(payload));

#if STORAGE_DEBUG
    LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): begin\n", page_addr);
#endif

    if (len > sizeof(payload.v2.payload)) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): buffer length out of payload size - %u > %d\n", page_addr, len, sizeof(payload.v2.payload));
#endif
        return STORAGE_ERROR_OUT_OF_MEMORY;
    }

    if (page_addr + sizeof(payload) - 1 > STORAGE_SIZE) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): unavailable page address\n", page_addr);
#endif
        return STORAGE_ERROR_OUT_OF_MEMORY;
    }

    eeprom_status_t status = eeprom_read(page_addr, (uint8_t*)&payload, sizeof(payload));
    if (status == EEPROM_ERROR_BUSY) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): eeprom read error - eeprom busy\n", page_addr);
#endif
        return STORAGE_ERROR_BUSY;
    }
    if (status != EEPROM_OK) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): eeprom read error\n", page_addr);
#endif
        return STORAGE_ERROR;
    }

    if (!_storage_check_inverse_bits(&payload)) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): bad inverse bits\n", page_addr);
#endif
        return STORAGE_ERROR_BITS;
    }

    if(payload.v2.header.magic != STORAGE_PAYLOAD_MAGIC) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): bad storage magic %08lX!=%08lX\n", page_addr, payload.v2.header.magic, STORAGE_PAYLOAD_MAGIC);
#endif
        return STORAGE_ERROR_BADACODE;
    }

    if(payload.v2.header.version != STORAGE_PAYLOAD_VERSION) {
#if STORAGE_DEBUG
        LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): bad storage version %i!=%i\n", page_addr, payload.v2.header.version, STORAGE_PAYLOAD_VERSION);
#endif
        return STORAGE_ERROR_VER;
    }

    if(payload.v2.header.appointment != appointment) {
#if STORAGE_DEBUG
		LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): wrong storage appointment %i!=%i\n", page_addr, payload.v2.header.appointment, appointment);
#endif
		return STORAGE_ERROR_APPOINTMENT;
	}

#if STORAGE_DEBUG
    LOG_DEBUG(STORAGE_TAG, "read page (address=%lu): OK\n", page_addr);
#endif

#if STORAGE_DEBUG
	util_debug_hex_dump(STORAGE_TAG, (uint8_t*)&payload, sizeof(payload));
#endif
    memcpy(buff, payload.v2.payload, len);
    return STORAGE_OK;
}

storage_status_t _storage_write_errors_list_page()
{
	is_error_page_updated = true;

#if STORAGE_DEBUG
    LOG_DEBUG(STORAGE_TAG, "write errors list page: begin\n");
    LOG_DEBUG(STORAGE_TAG, "write errors list page: WARNING! Trying to write errors bits\n");
#endif

    storage_status_t status = STORAGE_OK;
    uint32_t page_addr = STORAGE_START_ADDR;
	while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
		status = _storage_write_page(page_addr, (uint8_t*)&storage_errors_page_list, sizeof(storage_errors_page_list), STORAGE_APPOINTMENT_ERRORS_LIST_PAGE);
		if (status == STORAGE_ERROR_BUSY) {
			continue;
		}
		if (status != STORAGE_OK) {
#if STORAGE_DEBUG
			LOG_DEBUG(STORAGE_TAG, "write errors list page: unable to write errors list page (address=%lu)\n", page_addr);
#endif
			page_addr += STORAGE_PAGE_SIZE;
			continue;
		}

	    status = _storage_read_page(page_addr, (uint8_t*)&storage_errors_page_list, sizeof(storage_errors_page_list), STORAGE_APPOINTMENT_ERRORS_LIST_PAGE);
	    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
	        LOG_DEBUG(STORAGE_TAG, "write errors list page: unable to check errors list page\n");
#endif
			page_addr += STORAGE_PAGE_SIZE;
			continue;
	    }

		if (status == STORAGE_OK) {
			break;
		}
	}

#if STORAGE_DEBUG
    LOG_DEBUG(STORAGE_TAG, "write errors list page on adddress %lu: OK\n", page_addr);
#endif
    return status;
}

storage_status_t _storage_read_errors_list_page()
{
	if (!is_error_page_updated) {
	    return STORAGE_OK;
	}

#if STORAGE_DEBUG
    LOG_DEBUG(STORAGE_TAG, "read errors list page: begin\n");
#endif

    storage_status_t status = STORAGE_OK;
    uint32_t page_addr = STORAGE_START_ADDR;
    storage_errors_list_page_t search_buf;
    memset((uint8_t*)&search_buf, 0, sizeof(search_buf));
    while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
		status = _storage_read_page(page_addr, (uint8_t*)&search_buf, sizeof(search_buf), STORAGE_APPOINTMENT_ERRORS_LIST_PAGE);
		if (status == STORAGE_ERROR_BUSY) {
			continue;
		}
		if (status != STORAGE_OK) {
#if STORAGE_DEBUG
			LOG_DEBUG(STORAGE_TAG, "read errors list page: unable to read\n");
#endif
			page_addr += STORAGE_PAGE_SIZE;
			continue;
		}

		if (status == STORAGE_OK) {
			break;
		}
	}

    if (status == STORAGE_OK) {
        memcpy((uint8_t*)&storage_errors_page_list, (uint8_t*)&search_buf, sizeof(storage_errors_page_list));
        is_error_page_updated = false;
#if STORAGE_DEBUG
    	LOG_DEBUG(STORAGE_TAG, "read errors list page: OK\n");
#endif
    } else {
#if STORAGE_DEBUG
    	LOG_DEBUG(STORAGE_TAG, "read errors list page: error=%02x\n", status);
#endif
    }

    return status;
}

storage_status_t _storage_get_errors_list_page_addr(uint32_t* addr)
{
	storage_status_t status = STORAGE_OK;
	uint32_t page_addr = STORAGE_START_ADDR;
	storage_errors_list_page_t search_buf;
	memset((uint8_t*)&search_buf, 0, sizeof(search_buf));
	while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
		status = _storage_read_page(page_addr, (uint8_t*)&search_buf, sizeof(search_buf), STORAGE_APPOINTMENT_ERRORS_LIST_PAGE);
		if (status == STORAGE_ERROR_BUSY) {
			continue;
		}
		if (status != STORAGE_OK) {
#if STORAGE_DEBUG
			LOG_DEBUG(STORAGE_TAG, "read errors list page: unable to read\n");
#endif
			page_addr += STORAGE_PAGE_SIZE;
			continue;
		}

		if (status == STORAGE_OK) {
			*addr = page_addr;
			break;
		}
	}

	return status;
}

bool _storage_check_inverse_bits(storage_page_record_t* payload)
{
    for (uint32_t i = 0; i <  sizeof(payload->payload_bits); i++) {
        if ((payload->payload_bits[i] ^ 0xFF) != payload->invers_bits[i]) {
#if STORAGE_DEBUG
            LOG_DEBUG(STORAGE_TAG, "bad byte %lu check: %d!=%d\n", i, payload->payload_bits[i], payload->invers_bits[i]);
#endif
            return false;
        }
    }
    return true;
}

void _storage_inverse_payload_bits(storage_page_record_t* payload)
{
    for (uint32_t i = 0; i < sizeof(payload->payload_bits); i++) {
        payload->invers_bits[i] = (payload->payload_bits[i] ^ 0xFF);
    }
}

bool _storage_is_page_blocked(uint16_t page_num)
{
    uint32_t idx = page_num / 8;
    if (idx > sizeof(storage_errors_page_list.blocked)) {
        return true;
    }
    return (storage_errors_page_list.blocked[idx] >> (page_num % 8)) & 0x01;
}

void _storage_set_page_blocked(uint16_t page_num, bool blocked)
{
    uint32_t idx = page_num / 8;
    if (idx > sizeof(storage_errors_page_list.blocked)) {
        return;
    }
    if (blocked) {
        storage_errors_page_list.blocked[idx] |= (uint8_t)(0x01 << (page_num % 8));
    } else {
        storage_errors_page_list.blocked[idx] &= ~(uint8_t)(0x01 << (page_num % 8));
    }
}
