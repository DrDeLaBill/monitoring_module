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


typedef enum _storage_status_t {
	STORAGE_OK = 0,
	STORAGE_ERROR,
	STORAGE_ERROR_ADDR,
	STORAGE_ERROR_BITS,
	// End of memory
	STORAGE_EOM
} storage_status_t;


typedef struct __attribute__((packed)) _storage_payload_header_t {
	uint32_t magic; uint8_t version;
} storage_payload_header_t;


#define STORAGE_PAYLOAD_MAGIC    ((uint32_t)(0xBADAC0DE))
#define STORAGE_PAYLOAD_VERSION  (1)
#define STORAGE_MAX_PAYLOAD_SIZE (EEPROM_PAGE_SIZE)
#define STORAGE_PAGES_COUNT      (EEPROM_PAGE_COUNT)

#define STORAGE_PAYLOAD_BITS_SIZE(S) ( 			\
	S 										\
	- sizeof(storage_payload_header_t)	\
	- sizeof(uint16_t)						\
)


typedef struct __attribute__((packed)) _storage_payload_t {
	storage_payload_header_t header;
	uint8_t payload_bits[STORAGE_PAYLOAD_BITS_SIZE(STORAGE_MAX_PAYLOAD_SIZE) / 2];
	uint8_t invers_bits[STORAGE_PAYLOAD_BITS_SIZE(STORAGE_MAX_PAYLOAD_SIZE) / 2];
	uint16_t crc;
} storage_payload_t;


storage_status_t storage_load(uint32_t addr, uint8_t* data, uint16_t len);
storage_status_t storage_save(uint32_t addr, uint8_t* data, uint16_t len);
storage_status_t storage_get_first_available_addr(uint32_t* addr);
storage_status_t storage_get_second_available_addr(uint32_t* addr);
storage_status_t storage_get_next_available_addr(const uint32_t prev_addr, uint32_t* next_addr);


#ifdef __cplusplus
}
#endif


#endif /* INC_STORAGE_DATA_MANAGER_H_ */
