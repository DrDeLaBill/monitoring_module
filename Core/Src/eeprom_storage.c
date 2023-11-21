/* Copyright © 2023 Georgy E. All rights reserved. */

#include "eeprom_storage.h"

#include <stdbool.h>
#include <string.h>

#if defined(STM32F100xB) || \
    defined(STM32F100xE) || \
    defined(STM32F101x6) || \
    defined(STM32F101xB) || \
    defined(STM32F101xE) || \
    defined(STM32F101xG) || \
    defined(STM32F102x6) || \
    defined(STM32F102xB) || \
    defined(STM32F103x6) || \
    defined(STM32F103xB) || \
    defined(STM32F103xE) || \
    defined(STM32F103xG) || \
    defined(STM32F105xC) || \
    defined(STM32F107xC)
    #include "stm32f1xx_hal.h"
    #include "stm32f1xx_hal_i2c.h"
#elif defined(STM32F405xx) || \
    defined(STM32F415xx) || \
    defined(STM32F407xx) || \
    defined(STM32F417xx) || \
    defined(STM32F427xx) || \
    defined(STM32F437xx) || \
    defined(STM32F429xx) || \
    defined(STM32F439xx) || \
    defined(STM32F401xC) || \
    defined(STM32F401xE) || \
    defined(STM32F410Tx) || \
    defined(STM32F410Cx) || \
    defined(STM32F410Rx) || \
    defined(STM32F411xE) || \
    defined(STM32F446xx) || \
    defined(STM32F469xx) || \
    defined(STM32F479xx) || \
    defined(STM32F412Cx) || \
    defined(STM32F412Zx) || \
    defined(STM32F412Rx) || \
    defined(STM32F412Vx) || \
    defined(STM32F413xx) || \
    defined(STM32F423xx)
    #include "stm32f4xx_hal.h"
    #include "stm32f4xx_hal_i2c.h"
#else
    #error "Please select first the target STM32Fxxx device used in your application (in eeprom_at24cm01_storage.c file)"
#endif

#include "main.h"
#include "utils.h"


#define EEPROM_TIMER_DELAY ((uint16_t)GENERAL_BUS_TIMEOUT_MS)

const char* EEPROM_TAG = "EEPR";


eeprom_status_t eeprom_read(uint32_t addr, uint8_t* buf, uint16_t len)
{
#if EEPROM_DEBUG
    LOG_TAG_BEDUG(EEPROM_TAG, "eeprom read: begin (addr=%lu, length=%u)\n", addr, len);
#endif

    if (addr + len > EEPROM_PAGES_COUNT * EEPROM_PAGE_SIZE) {
#if EEPROM_DEBUG
        LOG_TAG_BEDUG(EEPROM_TAG, "eeprom read: error - out of max address or length\n");
#endif
        return EEPROM_ERROR_OOM;
    }

    uint8_t dev_addr = EEPROM_I2C_ADDR | (((addr >> 16) & 0x01) << 1);
#if EEPROM_DEBUG
    LOG_TAG_BEDUG(EEPROM_TAG, "eeprom read: device i2c address - 0x%02x\n", dev_addr);
#endif

    HAL_StatusTypeDef status = HAL_BUSY;
    util_timer_t timer;
    util_timer_start(&timer, GENERAL_BUS_TIMEOUT_MS);
    while (util_is_timer_wait(&timer)) {
        status = HAL_I2C_IsDeviceReady(&EEPROM_I2C, dev_addr, 1, GENERAL_BUS_TIMEOUT_MS);
        if (status == HAL_OK) {
            break;
        }
    }
    if (status != HAL_OK) {
        return EEPROM_ERROR_BUSY;
    }

    status = HAL_I2C_Mem_Read(&EEPROM_I2C, dev_addr, (uint16_t)(addr & 0xFFFF), I2C_MEMADD_SIZE_16BIT, buf, len, GENERAL_BUS_TIMEOUT_MS);
    if (status != HAL_OK) {
#if EEPROM_DEBUG
        LOG_TAG_BEDUG(EEPROM_TAG, "eeprom read: i2c error=0x%02x\n", status);
#endif
        return EEPROM_ERROR;
    }

#if EEPROM_DEBUG
    LOG_TAG_BEDUG(EEPROM_TAG, "eeprom read: OK\n");
#endif

    return EEPROM_OK;
}

eeprom_status_t eeprom_write(uint32_t addr, uint8_t* buf, uint16_t len)
{
#if EEPROM_DEBUG
    LOG_TAG_BEDUG(EEPROM_TAG, "eeprom write: begin (addr=%lu, length=%u)\n", addr, len);
#endif

    if (addr + len > EEPROM_PAGES_COUNT * EEPROM_PAGE_SIZE) {
#if EEPROM_DEBUG
        LOG_TAG_BEDUG(EEPROM_TAG, "eeprom write: error - out of max address or length\n");
#endif
        return EEPROM_ERROR_OOM;
    }

    uint8_t dev_addr = EEPROM_I2C_ADDR | (uint8_t)(((addr >> 16) & 0x01) << 1) | (uint8_t)1;
#if EEPROM_DEBUG
    LOG_TAG_BEDUG(EEPROM_TAG, "eeprom write: device i2c address - 0x%02x\n", dev_addr);
#endif

    HAL_StatusTypeDef status = HAL_BUSY;
    util_timer_t timer;
    util_timer_start(&timer, EEPROM_TIMER_DELAY);
    while (util_is_timer_wait(&timer)) {
        status = HAL_I2C_IsDeviceReady(&EEPROM_I2C, dev_addr, 1, GENERAL_BUS_TIMEOUT_MS);
        if (status == HAL_OK) {
            break;
        }
    }
    if (status != HAL_OK) {
        return EEPROM_ERROR_BUSY;
    }

    status = HAL_I2C_Mem_Write(&EEPROM_I2C, dev_addr, (uint16_t)(addr & 0xFFFF), I2C_MEMADD_SIZE_16BIT, buf, len, GENERAL_BUS_TIMEOUT_MS);
    if (status != HAL_OK) {
#if EEPROM_DEBUG
        LOG_TAG_BEDUG(EEPROM_TAG, "eeprom write: i2c error=0x%02x\n", status);
#endif
        return EEPROM_ERROR;
    }

#if EEPROM_DEBUG
    LOG_TAG_BEDUG(EEPROM_TAG, "eeprom write: OK\n");
#endif

    return EEPROM_OK;
}

uint32_t eeprom_get_size()
{
    return EEPROM_PAGE_SIZE * EEPROM_PAGES_COUNT;
}
