/*
 * eeprom_storage.h
 *
 *  Created on: Jul 25, 2023
 *      Author: DrDeLaBill
 */

#ifndef INC_EEPROM_STORAGE_H_
#define INC_EEPROM_STORAGE_H_


#include <stm32f1xx_hal.h>
#include <stdbool.h>


#define EEPROM_I2C_ADDR   ((uint8_t)0b10100000)
#define EEPROM_PAGE_SIZE  (256)
#define EEPROM_PAGE_COUNT (512)
#define EEPROM_DEBUG      (false)


typedef enum _eeprom_status_t {
	EEPROM_OK = 0x00,
	EEPROM_ERROR,
	EEPROM_ERROR_ADDR,
	EEPROM_ERROR_BUSY
} eeprom_status_t;


eeprom_status_t eeprom_read(uint32_t addr, uint8_t* buf, uint16_t len);
eeprom_status_t eeprom_write(uint32_t addr, uint8_t* buf, uint16_t len);


#endif /* INC_EEPROM_STORAGE_H_ */
