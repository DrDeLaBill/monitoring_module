/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _FLASH_STORAGE_H_
#define _FLASH_STORAGE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


#define FLASH_BEDUG               (0)

#define FLASH_TEST                (false)
#define FLASH_TEST_PAGES_COUNT    ((uint32_t)64)

#define FLASH_W25_PAGE_SIZE       ((uint32_t)0x100)
#define FLASH_W25_SECTOR_SIZE     ((uint32_t)0x1000)
#define FLASH_W25_SETORS_IN_BLOCK ((uint32_t)0x10)


typedef enum _flash_status_t {
    FLASH_OK    = ((uint8_t)0x00),  // OK
    FLASH_ERROR = ((uint8_t)0x01),  // Internal error
    FLASH_BUSY  = ((uint8_t)0x02),  // Memory or bus is busy
    FLASH_OOM   = ((uint8_t)0x03)   // Out Of Memory
} flash_status_t;


/**
 *  Initializes the W25Qxx chip.
 *  @return Result status.
 */
flash_status_t flash_w25qxx_init();

/**
 *  Completely clears the FLASH memory.
 *  @return Result status.
 */
flash_status_t flash_w25qxx_reset();

/**
 *  Reads data from the FLASH memory.
 *  @param addr Target read address.
 *  @param data Data buffer for read.
 *  @param len Data buffer length.
 *  @return Result status.
 */
flash_status_t flash_w25qxx_read(const uint32_t addr, uint8_t* data, const uint32_t len);

/**
 *  Writes data to the FLASH memory.
 *  @param addr Target read address.
 *  @param data Buffer with data for write.
 *  @param len Data buffer length (256 units maximum).
 *  @return Result status.
 */
flash_status_t flash_w25qxx_write(const uint32_t addr, const uint8_t* data, const uint32_t len);

/**
 *  Erases addresses in the FLASH memory.
 *  @param addrs[] Array of addresses.
 *  @param count   Number of the addresses.
 *  @return Result status.
 */
flash_status_t flash_w25qxx_erase_addresses(const uint32_t* addrs, const uint32_t count);

/**
 *  @return FLASH memory pages count.
 */
uint32_t flash_w25qxx_get_pages_count();

/**
 *  @return FLASH memory blocks count.
 */
uint32_t flash_w25qxx_get_blocks_count();

/**
 *  @return FLASH memory block size.
 */
uint32_t flash_w25qxx_get_block_size();


#ifdef __cplusplus
}
#endif


#endif
