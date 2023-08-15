/*
 * eeprom_storage.c
 *
 *  Created on: Jul 25, 2023
 *      Author: DrDeLaBill
 */
#include <eeprom_storage.h>

#include <stdbool.h>

#include <stm32f1xx_hal.h>
#include <stm32f1xx_hal_i2c.h>

#include "main.h"
#include "utils.h"


#define EEPROM_DELAY       ((uint16_t)0xFFFF)
#define EEPROM_TIMER_DELAY ((uint16_t)5000)

const char* EEPROM_TAG = "EEPR";


eeprom_status_t eeprom_read(uint32_t addr, uint8_t* buf, uint16_t len)
{
#if EEPROM_DEBUG
    LOG_DEBUG(EEPROM_TAG, "eeprom read: begin (addr=%lu, length=%u)\n", addr, len);
#endif

    if (addr > EEPROM_PAGE_COUNT * EEPROM_PAGE_SIZE) {
#if EEPROM_DEBUG
        LOG_DEBUG(EEPROM_TAG, "eeprom read: error - out of max address\n");
#endif
        return EEPROM_ERROR_ADDR;
    }
    if (addr + len > EEPROM_PAGE_COUNT * EEPROM_PAGE_SIZE) {
#if EEPROM_DEBUG
        LOG_DEBUG(EEPROM_TAG, "eeprom read: error - out of max address or length\n");
#endif
        return EEPROM_ERROR_ADDR;
    }

    uint8_t dev_addr = EEPROM_I2C_ADDR | (((addr >> 16) & 0x01) << 1);
#if EEPROM_DEBUG
    LOG_DEBUG(EEPROM_TAG, "eeprom read: device i2c address - 0x%02x\n", dev_addr);
#endif

    HAL_StatusTypeDef status;
    dio_timer_t timer;
    util_timer_start(&timer, EEPROM_TIMER_DELAY);
    while (util_is_timer_wait(&timer)) {
    	status = HAL_I2C_IsDeviceReady(&EEPROM_I2C, dev_addr, 1, HAL_MAX_DELAY);
    	if (status == HAL_OK) {
    		break;
    	}
    }
    if (status != HAL_OK) {
    	return EEPROM_ERROR_BUSY;
    }

    status = HAL_I2C_Mem_Read(&EEPROM_I2C, dev_addr, (uint16_t)(addr & 0xFFFF), I2C_MEMADD_SIZE_16BIT, buf, len, EEPROM_DELAY);
    if (status != HAL_OK) {
#if EEPROM_DEBUG
        LOG_DEBUG(EEPROM_TAG, "eeprom read: i2c error=0x%02x\n", status);
#endif
        return EEPROM_ERROR;
    }

#if EEPROM_DEBUG
    LOG_DEBUG(EEPROM_TAG, "eeprom read: OK\n");
#endif

    return EEPROM_OK;
}

eeprom_status_t eeprom_write(uint32_t addr, uint8_t* buf, uint16_t len)
{
#if EEPROM_DEBUG
    LOG_DEBUG(EEPROM_TAG, "eeprom write: begin (addr=%lu, length=%u)\n", addr, len);
#endif

    if (addr > EEPROM_PAGE_COUNT * EEPROM_PAGE_SIZE) {
#if EEPROM_DEBUG
        LOG_DEBUG(EEPROM_TAG, "eeprom write: error - out of max address\n");
#endif
        return EEPROM_ERROR_ADDR;
    }
    if (addr + len > EEPROM_PAGE_COUNT * EEPROM_PAGE_SIZE) {
#if EEPROM_DEBUG
        LOG_DEBUG(EEPROM_TAG, "eeprom write: error - out of max address or length\n");
#endif
        return EEPROM_ERROR_ADDR;
    }

    uint8_t dev_addr = EEPROM_I2C_ADDR | (uint8_t)(((addr >> 16) & 0x01) << 1) | (uint8_t)1;
#if EEPROM_DEBUG
    LOG_DEBUG(EEPROM_TAG, "eeprom write: device i2c address - 0x%02x\n", dev_addr);
#endif

    HAL_StatusTypeDef status;
    dio_timer_t timer;
    util_timer_start(&timer, EEPROM_TIMER_DELAY);
    while (util_is_timer_wait(&timer)) {
    	status = HAL_I2C_IsDeviceReady(&EEPROM_I2C, dev_addr, 1, HAL_MAX_DELAY);
    	if (status == HAL_OK) {
    		break;
    	}
    }
    if (status != HAL_OK) {
    	return EEPROM_ERROR_BUSY;
    }

    status = HAL_I2C_Mem_Write(&EEPROM_I2C, dev_addr, (uint16_t)(addr & 0xFFFF), I2C_MEMADD_SIZE_16BIT, buf, len, EEPROM_DELAY);
    if (status != HAL_OK) {
#if EEPROM_DEBUG
        LOG_DEBUG(EEPROM_TAG, "eeprom write: i2c error=0x%02x\n", status);
#endif
        return EEPROM_ERROR;
    }

#if EEPROM_DEBUG
    LOG_DEBUG(EEPROM_TAG, "eeprom write: OK\n");
#endif

    return EEPROM_OK;
}
