/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _EEPROM_AT24CM01_STORAGE_H_
#define _EEPROM_AT24CM01_STORAGE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


#define EEPROM_I2C_ADDR    ((uint8_t)0xA0)
#define EEPROM_PAGE_SIZE   (256)
#define EEPROM_PAGES_COUNT (512)
#define EEPROM_DEBUG       (false)


typedef enum _eeprom_status_t {
    EEPROM_OK = 0x00,
    EEPROM_ERROR,
    EEPROM_ERROR_OOM,
    EEPROM_ERROR_BUSY
} eeprom_status_t;


eeprom_status_t eeprom_read(uint32_t addr, uint8_t* buf, uint32_t len);
eeprom_status_t eeprom_write(uint32_t addr, uint8_t* buf, uint32_t len);
uint32_t        eeprom_get_size();


#ifdef __cplusplus
}
#endif


#endif
