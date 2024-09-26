/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageDriver.h"

#include "glog.h"
#include "soul.h"
#include "bmacro.h"

#include "StorageType.h"

#ifdef EEPROM_MODE
#   include "at24cm01.h"
#else
#   include "w25qxx.h"
#endif


#define ERROR_TIMEOUT_MS ((uint32_t)200)


bool StorageDriver::hasError = false;
utl::Timer StorageDriver::timer(ERROR_TIMEOUT_MS);

#if STORAGE_DRIVER_USE_BUFFER

bool StorageDriver::hasBuffer = false;
uint8_t StorageDriver::bufferPage[STORAGE_PAGE_SIZE] = {};
uint32_t StorageDriver::lastAddress = 0;

#endif


StorageStatus StorageDriver::read(const uint32_t address, uint8_t *data, const uint32_t len) {
#ifdef EEPROM_MODE
	if (is_error(POWER_ERROR) || is_status(MEMORY_ERROR)) {

#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Error power");
#endif

		return STORAGE_ERROR;
	}
	eeprom_status_t status = EEPROM_OK;

#if STORAGE_DRIVER_USE_BUFFER

	if (hasBuffer && lastAddress == address && len == STORAGE_PAGE_SIZE) {
		memcpy(data, bufferPage, len);

#	if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Copy %lu address start", address);
#	endif

	} else {

#endif

		status = eeprom_read(address, data, len);
#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Read %lu address start", address);
#endif

#if STORAGE_DRIVER_USE_BUFFER

	}

#endif
	if (hasError && !timer.wait()) {
		set_status(MEMORY_READ_FAULT);
	}
	if (!hasError && status != EEPROM_OK) {
		hasError = true;
		timer.start();
	}
#if STORAGE_DRIVER_BEDUG
    if (status != EEPROM_OK) {
		printTagLog(TAG, "Read %lu address error=%u", address, status);
    }
#endif
    if (status == EEPROM_ERROR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EEPROM_ERROR_OOM) {
        return STORAGE_OOM;
    }
    if (status != EEPROM_OK) {
        return STORAGE_ERROR;
    }

#if STORAGE_DRIVER_USE_BUFFER

    if (lastAddress != address && len == STORAGE_PAGE_SIZE) {
    	memcpy(bufferPage, data, STORAGE_PAGE_SIZE);
    	lastAddress = address;
    	hasBuffer = true;
    }

#endif

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Read %lu address success", address);
#endif

	hasError = false;
	reset_status(MEMORY_READ_FAULT);
    return STORAGE_OK;
#else
	if (is_error(POWER_ERROR) || is_status(MEMORY_ERROR)) {

#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Error power", address);
#endif

		return STORAGE_ERROR;
	}
	flash_status_t status = FLASH_OK;

#if STORAGE_DRIVER_USE_BUFFER

	if (hasBuffer && lastAddress == address && len == STORAGE_PAGE_SIZE) {
		memcpy(data, bufferPage, len);

#	if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Copy %lu address start", address);
#	endif

	} else {

#endif

		status = flash_w25qxx_read(address, data, len);
#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Read %lu address start", address);
#endif

#if STORAGE_DRIVER_USE_BUFFER

	}

#endif
	if (hasError && !timer.wait()) {
		set_status(MEMORY_READ_FAULT);
	}
	if (!hasError && status != FLASH_OK) {
		hasError = true;
		timer.start();
	}
#if STORAGE_DRIVER_BEDUG
    if (status != FLASH_OK) {
		printTagLog(TAG, "Read %lu address error=%u", address, status);
    }
#endif
    if (status == FLASH_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == FLASH_OOM) {
        return STORAGE_OOM;
    }
    if (status != FLASH_OK) {
        return STORAGE_ERROR;
    }

#if STORAGE_DRIVER_USE_BUFFER

    if (lastAddress != address && len == STORAGE_PAGE_SIZE) {
    	memcpy(bufferPage, data, STORAGE_PAGE_SIZE);
    	lastAddress = address;
    	hasBuffer = true;
    }

#endif

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Read %lu address success", address);
#endif

	hasError = false;
	reset_status(MEMORY_READ_FAULT);
    return STORAGE_OK;
#endif
}

StorageStatus StorageDriver::write(const uint32_t address, const uint8_t *data, const uint32_t len) {
#ifdef EEPROM_MODE
	if (is_error(POWER_ERROR) || is_status(MEMORY_ERROR)) {

#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Error power");
#endif

		return STORAGE_ERROR;
	}

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Write %lu address start", address);
#endif

	eeprom_status_t status = eeprom_write(address, data, len);

#if STORAGE_DRIVER_USE_BUFFER

	if (lastAddress == address) {
		hasBuffer = false;
	}

#endif

	if (hasError && !timer.wait()) {
    	set_status(MEMORY_WRITE_FAULT);
	}
	if (!hasError && status != EEPROM_OK) {
		hasError = true;
		timer.start();
	}
#if STORAGE_DRIVER_BEDUG
    if (status != EEPROM_OK) {
		printTagLog(TAG, "Write %lu address error=%u", address, status);
    }
#endif
    if (status == EEPROM_ERROR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EEPROM_ERROR_OOM) {
        return STORAGE_OOM;
    }
    if (status != EEPROM_OK) {
        return STORAGE_ERROR;
    }

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Write %lu address success", address);
#endif

	hasError = false;
	reset_status(MEMORY_WRITE_FAULT);
    return STORAGE_OK;
#else
	if (is_error(POWER_ERROR) || is_status(MEMORY_ERROR)) {

#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Error power", address);
#endif

		return STORAGE_ERROR;
	}

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Write %lu address start", address);
#endif

	flash_status_t status = flash_w25qxx_write(address, data, len);

#if STORAGE_DRIVER_USE_BUFFER

	if (lastAddress == address) {
		hasBuffer = false;
	}

#endif

	if (hasError && !timer.wait()) {
    	set_status(MEMORY_WRITE_FAULT);
	}
	if (!hasError && status != FLASH_OK) {
		hasError = true;
		timer.start();
	}
#if STORAGE_DRIVER_BEDUG
    if (status != FLASH_OK) {
		printTagLog(TAG, "Write %lu address error=%u", address, status);
    }
#endif
    if (status == FLASH_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == FLASH_OOM) {
        return STORAGE_OOM;
    }
    if (status != FLASH_OK) {
        return STORAGE_ERROR;
    }

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Write %lu address success", address);
#endif

	hasError = false;
	reset_status(MEMORY_WRITE_FAULT);
    return STORAGE_OK;
#endif
}

#ifdef EEPROM_MODE
StorageStatus StorageDriver::erase(const uint32_t*, const uint32_t)
#else
StorageStatus StorageDriver::erase(const uint32_t* addresses, const uint32_t count)
#endif
{
#ifdef EEPROM_MODE
	return STORAGE_OK;
#else

	if (is_error(POWER_ERROR) || is_status(MEMORY_ERROR)) {

#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Error power", address);
#endif

		return STORAGE_ERROR;
	}

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Erase addresses start");
#endif

	flash_status_t status = flash_w25qxx_erase_addresses(addresses, count);

	if (hasError && !timer.wait()) {
		set_status(MEMORY_WRITE_FAULT);
	}
	if (!hasError && status != FLASH_OK) {
		hasError = true;
		timer.start();
	}
#if STORAGE_DRIVER_BEDUG
	if (status != FLASH_OK) {
		printTagLog(TAG, "Erase addresses error=%u", status);
	}
#endif
	if (status == FLASH_BUSY) {
		return STORAGE_BUSY;
	}
	if (status == FLASH_OOM) {
		return STORAGE_OOM;
	}
	if (status != FLASH_OK) {
		return STORAGE_ERROR;
	}

#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Erase addresses success");
#endif

	hasError = false;
	reset_status(MEMORY_WRITE_FAULT);
	return STORAGE_OK;
#endif
}
