/* Copyright © 2024 Georgy E. All rights reserved. */

#include "at24cm01.h"

#include <stdbool.h>
#include <string.h>

#include "hal_defs.h"

#include "glog.h"
#include "main.h"
#include "gutils.h"


#define EEPROM_TIMER_DELAY_MS ((uint16_t)1000)
#define EEPROM_DELAY_MS       ((uint16_t)100)
#define EEPROM_MAX_ERRORS     (5)


const char EEPROM_TAG[] = "EEPR";


eeprom_status_t eeprom_read(const uint32_t addr, uint8_t* buf, const uint32_t len)
{
#if EEPROM_DEBUG
    printTagLog(EEPROM_TAG, "eeprom read: begin (addr=%lu, length=%lu)", addr, len);
#endif

    if (addr + len > EEPROM_PAGES_COUNT * EEPROM_PAGE_SIZE) {
#if EEPROM_DEBUG
        printTagLog(EEPROM_TAG, "eeprom read: error - out of max address or length");
#endif
        return EEPROM_ERROR_OOM;
    }

    uint8_t dev_addr = EEPROM_I2C_ADDR | (((addr >> 16) & 0x01) << 1);
#if EEPROM_DEBUG
    printTagLog(EEPROM_TAG, "eeprom read: device i2c address - 0x%02x", dev_addr);
#endif

    HAL_StatusTypeDef status = HAL_BUSY;
    util_old_timer_t timer = { 0 };
    util_old_timer_start(&timer, EEPROM_TIMER_DELAY_MS);
    while (util_old_timer_wait(&timer)) {
        status = HAL_I2C_IsDeviceReady(&EEPROM_I2C, dev_addr, 1, EEPROM_DELAY_MS);

        if (status == HAL_OK) {
            break;
        }
    }
    if (status != HAL_OK) {
#if EEPROM_DEBUG
        printTagLog(EEPROM_TAG, "eeprom wait ready: i2c error=0x%02x", status);
#endif
        return EEPROM_ERROR_BUSY;
    }

    status = HAL_I2C_Mem_Read(
		&EEPROM_I2C,
		dev_addr,
		(uint16_t)(addr & 0xFFFF),
		I2C_MEMADD_SIZE_16BIT,
		buf,
		(uint16_t)len,
		EEPROM_DELAY_MS
	);
    if (status != HAL_OK) {
#if EEPROM_DEBUG
        printTagLog(EEPROM_TAG, "eeprom read: i2c error=0x%02x", status);
#endif
        return EEPROM_ERROR;
    }

#if EEPROM_DEBUG
    printTagLog(EEPROM_TAG, "eeprom read: OK");
#endif

    return EEPROM_OK;
}

eeprom_status_t eeprom_write(const uint32_t addr, const uint8_t* buf, const uint32_t len)
{
#if EEPROM_DEBUG
    printTagLog(EEPROM_TAG, "eeprom write: begin (addr=%lu, length=%lu)", addr, len);
#endif

    if (addr + len > EEPROM_PAGES_COUNT * EEPROM_PAGE_SIZE) {
#if EEPROM_DEBUG
        printTagLog(EEPROM_TAG, "eeprom write: error - out of max address or length");
#endif
        return EEPROM_ERROR_OOM;
    }

    uint8_t dev_addr = EEPROM_I2C_ADDR | (uint8_t)(((addr >> 16) & 0x01) << 1) | (uint8_t)1;
#if EEPROM_DEBUG
    printTagLog(EEPROM_TAG, "eeprom write: device i2c address - 0x%02x", dev_addr);
#endif

    HAL_StatusTypeDef status = HAL_BUSY;
    util_old_timer_t timer = { 0 };
    util_old_timer_start(&timer, EEPROM_TIMER_DELAY_MS);
    while (util_old_timer_wait(&timer)) {
        status = HAL_I2C_IsDeviceReady(&EEPROM_I2C, dev_addr, 1, EEPROM_DELAY_MS);
        if (status == HAL_OK) {
            break;
        }
    }
    if (status != HAL_OK) {
#if EEPROM_DEBUG
        printTagLog(EEPROM_TAG, "eeprom wait ready: i2c error=0x%02x", status);
#endif
        return EEPROM_ERROR_BUSY;
    }

    status = HAL_I2C_Mem_Write(
		&EEPROM_I2C,
		dev_addr,
		(uint16_t)(addr & 0xFFFF),
		I2C_MEMADD_SIZE_16BIT,
		(uint8_t*)buf,
		(uint16_t)len,
		EEPROM_DELAY_MS
	);
    if (status != HAL_OK) {
#if EEPROM_DEBUG
        printTagLog(EEPROM_TAG, "eeprom write: i2c error=0x%02x", status);
#endif
        return EEPROM_ERROR;
    }

#if EEPROM_DEBUG
    printTagLog(EEPROM_TAG, "eeprom write: OK");
#endif

    return EEPROM_OK;
}

uint32_t eeprom_get_size()
{
    return EEPROM_PAGE_SIZE * EEPROM_PAGES_COUNT;
}
