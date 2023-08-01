/*
 * storage_data_manager.h
 *
 *  Created on: Jul 25, 2023
 *      Author: DrDeLaBill
 */

#ifndef INC_STORAGE_DATA_MANAGER_H_
#define INC_STORAGE_DATA_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#include "eeprom_storage.h"


#define STORAGE_DEBUG (false)


typedef enum _storage_status_t {
	STORAGE_OK                  = ((uint8_t)0x00),
	STORAGE_ERROR               = ((uint8_t)0x01),
	STORAGE_ERROR_ADDR          = ((uint8_t)0x02),
	STORAGE_ERROR_BLOCKED       = ((uint8_t)0x03),
	STORAGE_ERROR_BADACODE      = ((uint8_t)0x04),
	STORAGE_ERROR_VER           = ((uint8_t)0x05),
	STORAGE_ERROR_APPOINTMENT   = ((uint8_t)0x06),
	STORAGE_ERROR_BITS          = ((uint8_t)0x07),
	STORAGE_ERROR_OUT_OF_MEMORY = ((uint8_t)0x08)
} storage_status_t;

typedef enum _storage_appointment_t {
	STORAGE_APPOINTMENT_COMMON           = ((uint8_t)0x00),
	STORAGE_APPOINTMENT_ERRORS_LIST_PAGE = ((uint8_t)0x01)
} storage_appointment_t;


typedef struct __attribute__((packed)) _storage_payload_header_t {
	uint32_t magic; uint8_t version; uint8_t appointment;
} storage_payload_header_t;


#define STORAGE_PAYLOAD_MAGIC     ((uint32_t)(0xBADAC0DE))
#define STORAGE_PAYLOAD_VERSION   (2)
#define STORAGE_PAGE_SIZE         (EEPROM_PAGE_SIZE)
#define STORAGE_PAGES_COUNT       (EEPROM_PAGE_COUNT)
#define STORAGE_START_ADDR        ((uint32_t)(0))

#define STORAGE_PAYLOAD_BITS_SIZE (STORAGE_PAGE_SIZE / 2)
#define STORAGE_PAYLOAD_SIZE      (STORAGE_PAYLOAD_BITS_SIZE - sizeof(struct _storage_payload_header_t) - sizeof(uint16_t))
#define STORAGE_SIZE              (STORAGE_PAGE_SIZE * STORAGE_PAGES_COUNT)


typedef union _storage_page_record_t {
	struct __attribute__((packed)) {
		uint8_t payload_bits[STORAGE_PAYLOAD_BITS_SIZE];
		uint8_t invers_bits[STORAGE_PAYLOAD_BITS_SIZE];
	};
	struct __attribute__((packed)) {
		struct _storage_payload_header_t header;
		uint8_t payload[STORAGE_PAYLOAD_SIZE];
		uint16_t crc;
		uint8_t invers_bits[STORAGE_PAYLOAD_BITS_SIZE];
	} v2;
} storage_page_record_t;


typedef struct __attribute__((packed)) _storage_errors_list_page_t {
    uint8_t blocked[STORAGE_PAYLOAD_SIZE];
} storage_errors_list_page_t;


storage_status_t storage_load(uint32_t addr, uint8_t* data, uint16_t len);
storage_status_t storage_save(uint32_t addr, uint8_t* data, uint16_t len);
storage_status_t storage_get_first_available_addr(uint32_t* addr);
storage_status_t storage_get_next_available_addr(uint32_t prev_addr, uint32_t* next_addr);
storage_status_t storage_reset_errors_list_page();


#ifdef __cplusplus
}
#endif


#endif /* INC_STORAGE_DATA_MANAGER_H_ */
