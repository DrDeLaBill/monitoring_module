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


#define STORAGE_PAGE_ERRORS_ADDR ((uint32_t)(0))


struct __attribute__((packed)) _storage_pages_err_list_t {
	uint8_t blocked[STORAGE_PAGES_COUNT];
} storage_pages_err_list = {0};


const char* STORAGE_TAG = "STRG";


storage_status_t _storage_write_page(uint32_t page_addr, uint8_t* buff, uint16_t len);
storage_status_t _storage_read_page(uint32_t page_addr, uint8_t* buff, uint16_t len);

storage_status_t _storage_write_pages_err_list();
storage_status_t _storage_read_pages_err_list();

void _storage_inverse_payload_bits(storage_payload_t* payload);
bool _storage_check_payload_bits(storage_payload_t* payload);


storage_status_t storage_load(uint32_t addr, uint8_t* data, uint16_t len)
{
	storage_status_t status = _storage_read_pages_err_list();
	if (status != STORAGE_OK) {
		LOG_DEBUG(STORAGE_TAG, " unable to load, unable to get available addresses\n");
		return status;
	}

	if (storage_pages_err_list.blocked[addr / STORAGE_MAX_PAYLOAD_SIZE]) {
		LOG_DEBUG(STORAGE_TAG, " unavailable address: storage page error\n");
		return STORAGE_ERROR_ADDR;
	}

	status = _storage_read_page(addr, data, len);
	if (status == STORAGE_ERROR_BITS) {
		storage_pages_err_list.blocked[addr / STORAGE_MAX_PAYLOAD_SIZE] = true;
		_storage_write_pages_err_list(); // TODO: add check
	}
	if (status != STORAGE_OK) {
		LOG_DEBUG(STORAGE_TAG, " unable to load, unable to get available addresses\n");
		return status;
	}

	return STORAGE_OK;
}

storage_status_t storage_save(uint32_t addr, uint8_t* data, uint16_t len)
{
	storage_status_t status = _storage_read_pages_err_list();
	if (status != STORAGE_OK) {
		LOG_DEBUG(STORAGE_TAG, " unable to load, unable to get available addresses\n");
		return status;
	}
	if (storage_pages_err_list.blocked[addr / STORAGE_MAX_PAYLOAD_SIZE]) {
		LOG_DEBUG(STORAGE_TAG, " unavailable address: storage page blocked\n");
		return STORAGE_ERROR_ADDR;
	}

	status = _storage_write_page(addr, data, len);
	if (status != STORAGE_OK) {
		LOG_DEBUG(STORAGE_TAG, " unable to load: storage write error\n");
		return status;
	}

	return STORAGE_OK;
}

storage_status_t storage_get_first_available_addr(uint32_t* addr)
{
	storage_status_t status = _storage_read_pages_err_list();

	if (status != STORAGE_OK) {
		LOG_DEBUG(STORAGE_TAG, " try to write errors list page\n");
		memset((uint8_t*)&storage_pages_err_list, 0, sizeof(storage_pages_err_list));
		status = _storage_write_pages_err_list();
	}

	if (status != STORAGE_OK) {
		LOG_DEBUG(STORAGE_TAG, " unable to get first available address\n");
		return STORAGE_ERROR;
	}

	for (uint16_t i = 1; i < sizeof(storage_pages_err_list.blocked); i++) {
		if (!storage_pages_err_list.blocked[i]) {
			*addr = i * STORAGE_MAX_PAYLOAD_SIZE;
			LOG_DEBUG(STORAGE_TAG, " first address found: %lu\n", *addr);
			return STORAGE_OK;
		}
	}

	LOG_DEBUG(STORAGE_TAG, " there are no available addresses\n");
	return status;
}

storage_status_t storage_get_next_available_addr(const uint32_t prev_addr, uint32_t* next_addr)
{
	uint32_t tmp_addr = prev_addr + STORAGE_MAX_PAYLOAD_SIZE;
	if (tmp_addr > STORAGE_MAX_PAYLOAD_SIZE * (STORAGE_PAGES_COUNT - 1)) {
		LOG_DEBUG(STORAGE_TAG, " end of memory\n");
		return STORAGE_EOM;
	}

	for (uint16_t i = prev_addr / STORAGE_MAX_PAYLOAD_SIZE + 1; i < sizeof(storage_pages_err_list.blocked); i++) {
		if (!storage_pages_err_list.blocked[i]) {
			*next_addr = i * STORAGE_MAX_PAYLOAD_SIZE;
			LOG_DEBUG(STORAGE_TAG, " next address found: %lu\n", *next_addr);
			return STORAGE_OK;
		}
	}

	LOG_DEBUG(STORAGE_TAG, " there is no available next address\n");
	return STORAGE_ERROR;
}

storage_status_t _storage_write_page(uint32_t page_addr, uint8_t* buff, uint16_t len)
{
	storage_payload_t payload;
	memset((uint8_t*)&payload, 0, sizeof(payload));

	if (len > sizeof(payload.payload_bits)) {
		LOG_DEBUG(STORAGE_TAG, " buffer length out of range\n");
		return STORAGE_ERROR;
	}

	if (page_addr > STORAGE_MAX_PAYLOAD_SIZE * (STORAGE_PAGES_COUNT - 1)) {
		LOG_DEBUG(STORAGE_TAG, " unavailable page address\n");
		return STORAGE_ERROR;
	}

	payload.header.magic = STORAGE_PAYLOAD_MAGIC;
	payload.header.version = STORAGE_PAYLOAD_VERSION;

	memcpy(payload.payload_bits, buff, len);

	_storage_inverse_payload_bits(&payload);

	payload.crc = util_get_crc16((uint8_t*)&payload, sizeof(payload) - sizeof(payload.crc));

	eeprom_status_t eeprom_status = eeprom_write(page_addr, (uint8_t*)&payload, sizeof(payload));
	if (eeprom_status == EEPROM_ERROR) {
		storage_pages_err_list.blocked[page_addr / STORAGE_MAX_PAYLOAD_SIZE] = 1;
		_storage_write_pages_err_list();
	}
	if (eeprom_status != EEPROM_OK) {
		LOG_DEBUG(STORAGE_TAG, " eeprom write error\n");
		return STORAGE_ERROR;
	}

	LOG_DEBUG(STORAGE_TAG, " page saved\n");
	return STORAGE_OK;
}

storage_status_t _storage_read_page(uint32_t page_addr, uint8_t* buff, uint16_t len)
{
	storage_payload_t payload;
	memset((uint8_t*)&payload, 0, sizeof(payload));

	if (len > sizeof(payload.payload_bits)) {
		LOG_DEBUG(STORAGE_TAG, " buffer length out of range\n");
		return STORAGE_ERROR;
	}

	if (page_addr > STORAGE_MAX_PAYLOAD_SIZE * (STORAGE_PAGES_COUNT - 1)) {
		LOG_DEBUG(STORAGE_TAG, " unavailable page address\n");
		return STORAGE_ERROR_ADDR;
	}

	if (eeprom_read(page_addr, (uint8_t*)&payload, sizeof(payload)) != EEPROM_OK) {
		LOG_DEBUG(STORAGE_TAG, " eeprom read error\n");
		return STORAGE_ERROR;
	}

	if(payload.header.magic != STORAGE_PAYLOAD_MAGIC) {
		LOG_DEBUG(STORAGE_TAG, " bad storage magic %08lX!=%08lX\n", payload.header.magic, STORAGE_PAYLOAD_MAGIC);
		return STORAGE_ERROR;
	}

	if(payload.header.version != STORAGE_PAYLOAD_VERSION) {
		LOG_DEBUG(STORAGE_TAG, " bad storage version %i!=%i\n", payload.header.version, STORAGE_PAYLOAD_VERSION);
		return STORAGE_ERROR;
	}

	if (!_storage_check_payload_bits(&payload)) {
		return STORAGE_ERROR_BITS;
	}

	memcpy((uint8_t*)&buff, payload.payload_bits, len);
	LOG_DEBUG(STORAGE_TAG, " page read\n");
	return STORAGE_OK;
}

storage_status_t _storage_write_pages_err_list()
{
	storage_status_t status = _storage_write_page(STORAGE_PAGE_ERRORS_ADDR, (uint8_t*)&storage_pages_err_list, sizeof(storage_pages_err_list));
	if (status != STORAGE_OK) {
		LOG_DEBUG(STORAGE_TAG, " unable to write error pages list\n");
		return STORAGE_ERROR;
	}

	status = _storage_read_pages_err_list();
	if (status != STORAGE_OK) {
		LOG_DEBUG(STORAGE_TAG, " unable to check error pages list\n");
		return STORAGE_ERROR;
	}

	return STORAGE_OK;
}

storage_status_t _storage_read_pages_err_list()
{
	storage_status_t status = _storage_read_page(STORAGE_PAGE_ERRORS_ADDR, (uint8_t*)&storage_pages_err_list, sizeof(storage_pages_err_list));
	if (status != STORAGE_OK) {
		LOG_DEBUG(STORAGE_TAG, " unable to read error pages list\n");
		return STORAGE_ERROR;
	}

	return STORAGE_OK;
}

bool _storage_check_payload_bits(storage_payload_t* payload)
{
	for (uint32_t i = 0; i < sizeof(payload->payload_bits) / sizeof(*payload->payload_bits); i++) {
		if ((payload->payload_bits[i] ^ 0xFF) != payload->invers_bits[i]) {
			LOG_DEBUG(STORAGE_TAG, " bad byte %lu check: %d!=%d\n", i, payload->payload_bits[i], payload->invers_bits[i]);
			return false;
		}
	}
	return true;
}

void _storage_inverse_payload_bits(storage_payload_t* payload)
{
	for (uint32_t i = 0; i < STORAGE_PAYLOAD_BITS_SIZE(STORAGE_MAX_PAYLOAD_SIZE) / 2; i++) {
		payload->invers_bits[i] = (payload->payload_bits[i] ^ 0xFF);
	}
}
