/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageDriver.h"

#include "bmacro.h"
#include "eeprom_at24cm01_storage.h"


StorageStatus StorageDriver::read(uint32_t address, uint8_t *data, uint32_t len) {
    eeprom_status_t status = eeprom_read(address, data, len);
	BEDUG_ASSERT((status != EEPROM_ERROR_BUSY), "Storage is busy");
    if (status == EEPROM_ERROR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EEPROM_ERROR_OOM) {
        return STORAGE_OOM;
    }
    if (status != EEPROM_OK) {
        return STORAGE_ERROR;
    }
    return STORAGE_OK;
}
;

StorageStatus StorageDriver::write(uint32_t address, uint8_t *data, uint32_t len) {
	eeprom_status_t status = eeprom_write(address, data, len);
	BEDUG_ASSERT((status != EEPROM_ERROR_BUSY), "Storage is busy");
    if (status == EEPROM_ERROR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EEPROM_ERROR_OOM) {
        return STORAGE_OOM;
    }
    if (status != EEPROM_OK) {
        return STORAGE_ERROR;
    }
    return STORAGE_OK;
}
