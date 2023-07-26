/*
 * eeprom_storage.c
 *
 *  Created on: Jul 25, 2023
 *      Author: DrDeLaBill
 */
#include <eeprom_storage.h>

#include <stm32f1xx_hal.h>
#include <stm32f1xx_hal_i2c.h>

#include "main.h"


#define EEPROM_DELAY ((uint16_t)0xFFFF)


eeprom_status_t eeprom_read(uint32_t addr, uint8_t* buf, uint16_t len)
{
	if (addr > EEPROM_PAGE_COUNT * EEPROM_PAGE_SIZE) {
		return EEPROM_ERROR_ADDR;
	}
	if (addr + len > EEPROM_PAGE_COUNT * EEPROM_PAGE_SIZE) {
		return EEPROM_ERROR_ADDR;
	}

	uint8_t dev_addr = EEPROM_I2C_ADDR | (((addr >> 16) & 0x01) << 1);

	HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(&EEPROM_I2C, EEPROM_I2C_ADDR, 1, EEPROM_DELAY);
	if (status != HAL_OK) {
		return EEPROM_ERROR_BUSY;
	}

	status = HAL_I2C_Mem_Read(&EEPROM_I2C, dev_addr, (uint16_t)(addr & 0xFFFF), I2C_MEMADD_SIZE_16BIT, buf, len, EEPROM_DELAY);
	if (status != HAL_OK) {
		return EEPROM_ERROR;
	}

	return EEPROM_OK;
}

eeprom_status_t eeprom_write(uint32_t addr, uint8_t* buf, uint16_t len)
{
	if (addr > EEPROM_PAGE_COUNT * EEPROM_PAGE_SIZE) {
		return EEPROM_ERROR_ADDR;
	}
	if (addr + len > EEPROM_PAGE_COUNT * EEPROM_PAGE_SIZE) {
		return EEPROM_ERROR_ADDR;
	}

	uint8_t dev_addr = EEPROM_I2C_ADDR | (uint8_t)(((addr >> 16) & 0x01) << 1) | (uint8_t)1;

	HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(&EEPROM_I2C, EEPROM_I2C_ADDR, 1, EEPROM_DELAY);
	if (status != HAL_OK) {
		return EEPROM_ERROR_BUSY;
	}

	status = HAL_I2C_Mem_Write(&EEPROM_I2C, dev_addr, (uint16_t)(addr & 0xFFFF), I2C_MEMADD_SIZE_16BIT, buf, len, EEPROM_DELAY);
	if (status != HAL_OK) {
		return EEPROM_ERROR;
	}

	return EEPROM_OK;
}
