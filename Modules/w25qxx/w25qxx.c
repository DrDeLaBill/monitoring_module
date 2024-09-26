/* Copyright © 2023 Georgy E. All rights reserved. */

#include "w25qxx.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "glog.h"
#include "soul.h"
#include "main.h"
#include "gutils.h"
#include "hal_defs.h"


typedef enum _flash_w25_command_t {
    FLASH_W25_CMD_WRITE_SR1       = ((uint8_t)0x01),
    FLASH_W25_CMD_PAGE_PROGRAMM   = ((uint8_t)0x02),
    FLASH_W25_CMD_READ            = ((uint8_t)0x03),
    FLASH_W25_CMD_WRITE_DISABLE   = ((uint8_t)0x04),
    FLASH_W25_CMD_READ_SR1        = ((uint8_t)0x05),
    FLASH_W25_CMD_WRITE_ENABLE    = ((uint8_t)0x06),
    FLASH_W25_CMD_ERASE_SECTOR    = ((uint8_t)0x20),
    FLASH_W25_CMD_WRITE_ENABLE_SR = ((uint8_t)0x50),
    FLASH_W25_CMD_ENABLE_RESET    = ((uint8_t)0x66),
    FLASH_W25_CMD_RESET           = ((uint8_t)0x99),
    FLASH_W25_CMD_JEDEC_ID        = ((uint8_t)0x9f)
} flash_w25_command_t;


typedef struct _flash_info_t {
    bool     initialized;
    bool     is_24bit_address;

    uint32_t page_size;
    uint32_t pages_count;

    uint32_t sector_size;
    uint32_t sectors_in_block;

    uint32_t block_size;
    uint32_t blocks_count;
} flash_info_t;


#define FLASH_W25_JEDEC_ID_SIZE       (sizeof(uint32_t))
#define FLASH_W25_SR1_BUSY            ((uint8_t)0x01)
#define FLASH_W25_SR1_WEL             ((uint8_t)0x02)
#define FLASH_W25_24BIT_ADDR_SIZE     ((uint16_t)512)
#define FLASH_W25_SR1_UNBLOCK_VALUE   ((uint8_t)0x00)
#define FLASH_W25_SR1_BLOCK_VALUE     ((uint8_t)0x0F)

#define FLASH_SPI_TIMEOUT_MS          ((uint32_t)500)
#define FLASH_SPI_COMMAND_SIZE_MAX    ((uint8_t)10)



flash_status_t _flash_read_jdec_id(uint32_t* jdec_id);
flash_status_t _flash_read_SR1(uint8_t* SR1);

flash_status_t _flash_write_enable();
flash_status_t _flash_write_disable();
flash_status_t _flash_write(const uint32_t addr, const uint8_t* data, const uint32_t len);
flash_status_t _flash_set_protect_block(uint8_t value);

flash_status_t _flash_read(uint32_t addr, uint8_t* data, uint32_t len);

flash_status_t _flash_erase_sector(uint32_t addr);

flash_status_t _flash_data_cmp(const uint32_t addr, const uint8_t* data, const uint32_t len, bool* cmp_res);

flash_status_t _flash_send_data(const uint8_t* data, const uint16_t len);
flash_status_t _flash_recieve_data(uint8_t* data, uint16_t len);
void           _flash_spi_cs_set();
void           _flash_spi_cs_reset();

bool           _flash_check_FREE();
bool           _flash_check_WEL();

uint32_t       _flash_get_storage_bytes_size();


#ifdef DEBUG
const char FLASH_TAG[] = "FLSH";
#endif


#define FLASH_W25_JDEC_ID_BLOCK_COUNT_MASK ((uint16_t)0x4011)
const uint16_t w25qxx_jdec_id_block_count[] = {
    2,   // w25q10
    4,   // w25q20
    8,   // w25q40
    16,  // w25q80
    32,  // w25q16
    64,  // w25q32
    128, // w25q64
    256, // w25q128
    512, // w25q256
    1024 // w25q512
};


flash_info_t flash_info = {
    .initialized      = false,
    .is_24bit_address = false,

    .page_size        = FLASH_W25_PAGE_SIZE,
    .pages_count      = FLASH_W25_SECTOR_SIZE / FLASH_W25_PAGE_SIZE,

    .sector_size      = FLASH_W25_SECTOR_SIZE,
    .sectors_in_block = FLASH_W25_SETORS_IN_BLOCK,

    .block_size       = FLASH_W25_SETORS_IN_BLOCK * FLASH_W25_SECTOR_SIZE,
    .blocks_count     = 0
};


flash_status_t flash_w25qxx_init()
{
#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "flash init: begin");
#endif

    uint32_t jdec_id = 0;
    flash_status_t status = _flash_read_jdec_id(&jdec_id);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash init: error=%u (read JDEC ID)", status);
#endif
        goto do_spi_stop;
    }
    if (!jdec_id) {
    	status = FLASH_ERROR;
    	goto do_spi_stop;
    }

    flash_info.blocks_count = 0;
    uint16_t jdec_id_2b = (uint16_t)jdec_id;
    for (uint16_t i = 0; i < __arr_len(w25qxx_jdec_id_block_count); i++) {
        if ((uint16_t)(FLASH_W25_JDEC_ID_BLOCK_COUNT_MASK + i) == jdec_id_2b) {
            flash_info.blocks_count = w25qxx_jdec_id_block_count[i];
            break;
        }
    }

    if (!flash_info.blocks_count) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash init: error - unknown JDEC ID");
#endif
    	status = FLASH_ERROR;
        goto do_spi_stop;
    }


#if FLASH_BEDUG
    printTagLog(FLASH_TAG, "flash JDEC ID found: id=%08X, blocks_count=%lu", (unsigned int)jdec_id, flash_info.blocks_count);
#endif

	_flash_spi_cs_set();
    status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash init: error=%u (block FLASH error)", status);
#endif
        goto do_spi_stop;
    }
	_flash_spi_cs_reset();

    flash_info.initialized      = true;
    flash_info.is_24bit_address = (flash_info.blocks_count >= FLASH_W25_24BIT_ADDR_SIZE) ? true : false;

#if FLASH_BEDUG
    printTagLog(FLASH_TAG, "flash init: OK");
#endif

do_spi_stop:
	_flash_spi_cs_reset();

    return status;
}

flash_status_t flash_w25qxx_reset()
{
#if FLASH_BEDUG
    printTagLog(FLASH_TAG, "flash reset: begin");
#endif

	_flash_spi_cs_set();
    flash_status_t status = _flash_set_protect_block(FLASH_W25_SR1_UNBLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error=%u (unset block protect)", status);
#endif
        status = FLASH_BUSY;
        goto do_block_protect;
    }
	_flash_spi_cs_reset();

    uint8_t spi_cmd[] = { FLASH_W25_CMD_ENABLE_RESET, FLASH_W25_CMD_RESET };

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error (FLASH busy)");
#endif
        goto do_block_protect;
    }

	_flash_spi_cs_set();
    status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error=%u (send command)", status);
#endif
        status = FLASH_BUSY;
    }
	_flash_spi_cs_reset();

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error (flash is busy)");
#endif
        goto do_block_protect;
    }

    flash_status_t tmp_status = FLASH_OK;
do_block_protect:
	_flash_spi_cs_set();
    tmp_status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status == FLASH_OK) {
        status = tmp_status;
    } else {
        return status;
    }

	_flash_spi_cs_reset();

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error=%u (set block protected)", status);
#endif
        status = FLASH_BUSY;
    } else {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: OK");
#endif
    }

    return status;
}

flash_status_t flash_w25qxx_read(const uint32_t addr, uint8_t* data, const uint32_t len)
{
#if FLASH_BEDUG
//	printTagLog(FLASH_TAG, "flash read addr=%08lX len=%lu: begin", addr, len);
#endif

    if (!flash_info.initialized) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%08lX len=%lu (flash was not initialized)", addr, len);
#endif
    	return FLASH_ERROR;
    }

    _flash_spi_cs_set();

    flash_status_t status = _flash_read(addr, data, len);

	_flash_spi_cs_reset();

#if FLASH_BEDUG
//    if (status == FLASH_OK) {
//		printTagLog(FLASH_TAG, "flash read addr=%08lX len=%lu: OK", addr, len);
//    }
//    if (status == FLASH_OK && len) {
//		util_debug_hex_dump(data, addr, len);
//    }
#endif

    return status;
}

flash_status_t flash_w25qxx_write(const uint32_t addr, const uint8_t* data, const uint32_t len)
{
	/* Check input data BEGIN */
#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu: begin", addr, len);
//	util_debug_hex_dump(data, addr, len);
#endif

    if (!flash_info.initialized) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu (flash was not initialized)", addr, len);
#endif
        return FLASH_ERROR;
    }

	_flash_spi_cs_set();
	flash_status_t status = FLASH_OK;
    if (addr + len > _flash_get_storage_bytes_size()) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error (unacceptable address)", addr, len);
#endif
        status = FLASH_OOM;
        goto do_spi_stop;
    }
	_flash_spi_cs_reset();
	/* Check input data END */


    /* Compare old flashed data BEGIN */
	_flash_spi_cs_set();
    bool compare_status = false;
    status = _flash_data_cmp(addr, data, len, &compare_status);
	if (status != FLASH_OK) {
#if FLASH_BEDUG
		printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (compare data)", addr, len, status);
#endif
        goto do_spi_stop;
	}
	_flash_spi_cs_reset();

	if (!compare_status) {
#if FLASH_BEDUG
		printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu: ABORT (already written)", addr, len);
#endif
        return FLASH_OK;
	}
    /* Compare old flashed data END */

	/* Erase data BEGIN */
	{
		uint32_t erase_addrs[FLASH_W25_SECTOR_SIZE / FLASH_W25_PAGE_SIZE] = {0};
		unsigned erase_cnt        = 0;
		uint32_t erase_len        = 0;
		uint32_t erase_addr       = addr;
		erase_addrs[erase_cnt++] = erase_addr;
		while (erase_len < len) {
			uint32_t erase_next_addr        = erase_addr + FLASH_W25_PAGE_SIZE;
			uint32_t min_erase_size         = FLASH_W25_SECTOR_SIZE;
			uint32_t erase_sector_addr      = (erase_addr / min_erase_size) * min_erase_size;
			uint32_t erase_next_sector_addr = (erase_next_addr / min_erase_size) * min_erase_size;

			if (erase_len + FLASH_W25_PAGE_SIZE >= len ||
				erase_sector_addr != erase_next_sector_addr
			) {
				status = flash_w25qxx_erase_addresses(erase_addrs, erase_cnt);
				if (status != FLASH_OK) {
#if FLASH_BEDUG
					printTagLog(
						FLASH_TAG,
						"flash write addr=%08lX len=%lu error=%u (unable to erase old data)",
						addr,
						len,
						status
					);
#endif
				}

				memset(
					(uint8_t*)erase_addrs,
					0,
					sizeof(erase_addrs)
				);

				if (status == FLASH_BUSY) {
			        return status;
				}
				if (status != FLASH_OK) {
					break;
				}

				erase_cnt = 0;
			}

			erase_addr = erase_next_addr;
			erase_len += FLASH_W25_PAGE_SIZE;
			if (erase_len < len) {
				erase_addrs[erase_cnt++] = erase_next_addr;
			}
		}

		if (erase_cnt) {
			status = flash_w25qxx_erase_addresses(erase_addrs, erase_cnt);
		}
		if (status != FLASH_OK) {
#if FLASH_BEDUG
			printTagLog(
				FLASH_TAG,
				"flash write addr=%08lX len=%lu error=%u (unable to erase old data)",
				addr,
				len,
				status
			);
#endif
		}
	}
	/* Erase data END */


    /* Write data BEGIN */
    uint32_t cur_len = 0;
    while (cur_len < len) {
    	uint32_t write_len = FLASH_W25_PAGE_SIZE;
    	if (cur_len + write_len > len) {
    		write_len = len - cur_len;
    	}
    	_flash_spi_cs_set();
    	status = _flash_write(addr + cur_len, data + cur_len, write_len);
    	_flash_spi_cs_reset();
    	if (status != FLASH_OK) {
#if FLASH_BEDUG
        	printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (write)", addr + cur_len, write_len, status);
#endif
            goto do_spi_stop;
    	}

    	_flash_spi_cs_set();
    	uint8_t page_buf[FLASH_W25_PAGE_SIZE] = {0};
		status = _flash_read(addr + cur_len, page_buf, write_len);
    	_flash_spi_cs_reset();
    	if (status != FLASH_OK) {
#if FLASH_BEDUG
        	printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (read written page after write)", addr + cur_len, write_len, status);
#endif
            goto do_spi_stop;
    	}

    	int cmp_res = memcmp(page_buf, data + cur_len, write_len);
		if (cmp_res) {
#if FLASH_BEDUG
        	printTagLog(
				FLASH_TAG,
				"flash write addr=%08lX len=%lu error=%d (compare written page with read)",
				addr + cur_len,
				write_len,
				cmp_res
			);
			printTagLog(FLASH_TAG, "Needed page:");
			util_debug_hex_dump(data + cur_len, addr + cur_len, write_len);
			printTagLog(FLASH_TAG, "Readed page:");
			util_debug_hex_dump(page_buf, addr + cur_len, write_len);
#endif
			set_error(EXPECTED_MEMORY_ERROR);
			status = FLASH_ERROR;
	        goto do_spi_stop;
    	}

		reset_error(EXPECTED_MEMORY_ERROR);

    	cur_len += write_len;
    }
    /* Write data END */

#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu: OK", addr, len);
#endif

do_spi_stop:
	_flash_spi_cs_reset();

    return status;
}

flash_status_t flash_w25qxx_erase_addresses(const uint32_t* addrs, const uint32_t count)
{
	if (!addrs) {
#if FLASH_BEDUG
		printTagLog(FLASH_TAG, "erase flash addresses error: addresses=NULL");
#endif
		return FLASH_ERROR;
	}

	if (!count) {
#if FLASH_BEDUG
		printTagLog(FLASH_TAG, "erase flash addresses error: count=%lu", count);
#endif
		return FLASH_ERROR;
	}

#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "erase flash addresses: ")
	for (uint32_t i = 0; i < count; i++) {
		gprint("%08lX ", addrs[i]);
	}
	gprint("\n");
#endif

	for (uint32_t i = 0; i < count;) {
		uint32_t cur_sector_idx  = addrs[i] / FLASH_W25_SECTOR_SIZE;
		uint32_t cur_sector_addr = cur_sector_idx * FLASH_W25_SECTOR_SIZE;
		uint8_t  sector_buf[FLASH_W25_SECTOR_SIZE] = {0};

		/* Addresses for erase in current sector BEGIN */
		uint32_t next_sector_i = 0;
		for (uint32_t j = i; j < count; j++) {
			if (addrs[j] / FLASH_W25_SECTOR_SIZE != cur_sector_idx) {
				next_sector_i = j;
				break;
			}
		}
		if (!next_sector_i) {
			next_sector_i = count;
		}
		/* Addresses for erase in current sector END */

		/* Read target sector BEGIN */
		_flash_spi_cs_set();
		flash_status_t status = _flash_read(cur_sector_addr, sector_buf, sizeof(sector_buf));
		_flash_spi_cs_reset();
		if (status != FLASH_OK) {
#if FLASH_BEDUG
			printTagLog(
				FLASH_TAG,
				"flash erase data addr=%08lX error (unable to read sector: block_idx=%lu sector_idx=%lu len=%lu)",
				cur_sector_addr,
				cur_sector_addr / flash_info.block_size,
				(cur_sector_addr % flash_info.block_size) / flash_info.sector_size,
				FLASH_W25_SECTOR_SIZE
			);
#endif
			return status;
		}
		/* Read target sector END */


		/* Check target sector need erase BEGIN */
		bool need_erase_sector = false;
		for (uint32_t j = i; j < next_sector_i; j++) {
			uint32_t addr_in_sector = addrs[j] % FLASH_W25_SECTOR_SIZE;
			for (uint32_t k = 0; k < FLASH_W25_PAGE_SIZE; k++) {
				if (sector_buf[addr_in_sector + k] != 0xFF) {
					need_erase_sector = true;
					break;
				}
			}
			if (need_erase_sector) {
				break;
			}
		}
		if (!need_erase_sector) {
#if FLASH_BEDUG
			for (uint32_t j = i; j < next_sector_i; j++) {
				printTagLog(
					FLASH_TAG,
					"flash address=%08X (sector addr=%08lX) already empty",
					addrs[j],
					cur_sector_addr
				);
			}
#endif
			i = next_sector_i;
			continue;
		}
		/* Check target sector need erase END */


		/* Erase sector BEGIN */
		_flash_spi_cs_set();
		status = _flash_erase_sector(cur_sector_addr);
		_flash_spi_cs_reset();
		if (status != FLASH_OK) {
#if FLASH_BEDUG
			printTagLog(
				FLASH_TAG,
				"flash erase data addr=%08lX error (unable to erase sector: block_addr=%08lX sector_addr=%08lX len=%lu)",
				cur_sector_addr,
				cur_sector_addr / flash_info.block_size,
				(cur_sector_addr % flash_info.block_size) / flash_info.sector_size,
				FLASH_W25_SECTOR_SIZE
			);
#endif
			return status;
		}
		_flash_spi_cs_set();
		if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
			_flash_spi_cs_reset();
#if FLASH_BEDUG
			printTagLog(FLASH_TAG, "flash erase data addr=%08lX error (flash is busy)", cur_sector_addr);
#endif
			return FLASH_BUSY;
		}
		_flash_spi_cs_reset();
		/* Erase sector END */


		/* Return old data BEGIN */
		for (unsigned j = 0; j < FLASH_W25_SECTOR_SIZE / FLASH_W25_PAGE_SIZE; j++) {
			bool need_restore = true;
			uint32_t tmp_page_addr = cur_sector_addr + j * FLASH_W25_PAGE_SIZE;
			for (unsigned k = i; k < next_sector_i; k++) {
				if (addrs[k] == tmp_page_addr) {
					need_restore = false;
				}
			}
			if (!need_restore) {
#if FLASH_BEDUG
				printTagLog(
					FLASH_TAG,
					"flash restore data addr=%08lX ignored",
					tmp_page_addr
				);
#endif
				continue;
			}

			need_restore = false;
			for (unsigned k = 0; k < FLASH_W25_PAGE_SIZE; k++) {
				if (sector_buf[tmp_page_addr % FLASH_W25_SECTOR_SIZE + k] != 0xFF) {
					need_restore = true;
					break;
				}
			}
			if (!need_restore) {
#if FLASH_BEDUG
				printTagLog(
					FLASH_TAG,
					"flash restore data addr=%08lX ignored (page empty)",
					tmp_page_addr
				);
#endif
				continue;
			}

#if FLASH_BEDUG
			printTagLog(
				FLASH_TAG,
				"flash restore data addr=%08lX begin",
				tmp_page_addr
			);
#endif
			_flash_spi_cs_set();
			status = _flash_write(
				tmp_page_addr,
				&sector_buf[tmp_page_addr % FLASH_W25_SECTOR_SIZE],
				FLASH_W25_PAGE_SIZE
			);
			_flash_spi_cs_reset();
			if (status != FLASH_OK) {
#if FLASH_BEDUG
				printTagLog(
					FLASH_TAG,
					"flash erase data addr=%08lX error=%u (unable to write old data page addr=%08lX)",
					cur_sector_addr,
					status,
					tmp_page_addr
				);
#endif
				return status;
			}

	    	uint8_t page_buf[FLASH_W25_PAGE_SIZE] = {0};
	    	_flash_spi_cs_set();
			status = _flash_read(tmp_page_addr, page_buf, FLASH_W25_PAGE_SIZE);
	    	_flash_spi_cs_reset();
	    	if (status != FLASH_OK) {
	#if FLASH_BEDUG
	        	printTagLog(
					FLASH_TAG,
					"flash erase data addr=%08lX error=%u (unable to read page addr=%08lX)",
					cur_sector_addr,
					status,
					tmp_page_addr
				);
	#endif
				return status;
	    	}

	    	int cmp_res = memcmp(
				page_buf,
				&sector_buf[tmp_page_addr % FLASH_W25_SECTOR_SIZE],
				FLASH_W25_PAGE_SIZE
			);
			if (cmp_res) {
#if FLASH_BEDUG
	        	printTagLog(
					FLASH_TAG,
					"flash erase data addr=%08lX error=%d (compare written page with read)",
					tmp_page_addr,
					status
				);
				printTagLog(FLASH_TAG, "Needed page:");
				util_debug_hex_dump(
					&sector_buf[tmp_page_addr % FLASH_W25_SECTOR_SIZE],
					tmp_page_addr,
					FLASH_W25_PAGE_SIZE
				);
				printTagLog(FLASH_TAG, "Readed page:");
				util_debug_hex_dump(
					page_buf,
					tmp_page_addr,
					FLASH_W25_PAGE_SIZE
				);
#endif
				set_error(EXPECTED_MEMORY_ERROR);
				status = FLASH_ERROR;
				return status;
	    	}

#if FLASH_BEDUG
			printTagLog(
				FLASH_TAG,
				"flash restore data addr=%08lX OK",
				tmp_page_addr
			);
#endif

			reset_error(EXPECTED_MEMORY_ERROR);
		}
		/* Return old data END */

		i = next_sector_i;
	}

	return FLASH_OK;
}

flash_status_t _flash_write(const uint32_t addr, const uint8_t* data, const uint32_t len)
{
	if (len > flash_info.page_size) {
#if FLASH_BEDUG
		printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error (unacceptable data length)", addr, len);
#endif
		return FLASH_ERROR;
	}

    if (addr + len > _flash_get_storage_bytes_size()) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error (unacceptable address)", addr, len);
#endif
        return FLASH_OOM;
    }

    flash_status_t status = _flash_set_protect_block(FLASH_W25_SR1_UNBLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (unset block protect)", addr, len, status);
#endif
        goto do_block_protect;
    }
    status = _flash_write_enable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (write enable)", addr, len, status);
#endif
        goto do_block_protect;
    }
    if (!util_wait_event(_flash_check_WEL, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (WEL bit wait time exceeded)", addr, len, status);
#endif
		status = FLASH_BUSY;
		goto do_block_protect;
    }

    uint8_t counter = 0;
    uint8_t spi_cmd[FLASH_SPI_COMMAND_SIZE_MAX] = { 0 };

    spi_cmd[counter++] = FLASH_W25_CMD_PAGE_PROGRAMM;
    if (flash_info.is_24bit_address) {
        spi_cmd[counter++] = (addr >> 24) & 0xFF;
    }
    spi_cmd[counter++] = (addr >> 16) & 0xFF;
    spi_cmd[counter++] = (addr >> 8) & 0xFF;
    spi_cmd[counter++] = addr & 0xFF;

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error (FLASH busy)", addr, len);
#endif
        return FLASH_ERROR;
    }

    status = _flash_send_data(spi_cmd, counter);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (send command)", addr, len, (unsigned int)status);
#endif
		goto do_block_protect;
    }

    status = _flash_send_data(data, len);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
    	printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (wait write data timeout)", addr, len, (unsigned int)status);
#endif
		goto do_block_protect;
    }

do_block_protect:
    status = _flash_write_disable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (write is not disabled)", addr, len, status);
#endif
    }

    status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%08lX len=%lu error=%u (set block protected)", addr, len, status);
#endif
    }

    return status;
}

uint32_t flash_w25qxx_get_pages_count()
{
#if FLASH_TEST
	return FLASH_TEST_PAGES_COUNT;
#endif
    flash_status_t status = FLASH_OK;
    if (!flash_info.initialized) {
        status = flash_w25qxx_init();
    }
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get pages count: initializing error");
#endif
        return 0;
    }
    return flash_info.pages_count * flash_info.sectors_in_block * flash_info.blocks_count;
}

uint32_t flash_w25qxx_get_blocks_count()
{
#if FLASH_TEST
	return 1;
#endif
    flash_status_t status = FLASH_OK;
    if (!flash_info.initialized) {
        status = flash_w25qxx_init();
    }
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get blocks count: initializing error");
#endif
        return 0;
    }
    return flash_info.blocks_count;
}

uint32_t flash_w25qxx_get_block_size()
{
    flash_status_t status = FLASH_OK;
    if (!flash_info.initialized) {
        status = flash_w25qxx_init();
    }
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get block size: initializing error");
#endif
        return 0;
    }
    return flash_info.block_size;
}

flash_status_t _flash_data_cmp(const uint32_t addr, const uint8_t* data, const uint32_t len, bool* cmp_res)
{
	*cmp_res = false;

	uint32_t cur_len = 0;
	while (cur_len < len) {
		uint32_t needed_len = FLASH_W25_PAGE_SIZE;
		if (cur_len + needed_len > len) {
			needed_len = len - cur_len;
		}

		uint8_t read_data[FLASH_W25_PAGE_SIZE] = {0};
		flash_status_t status = _flash_read(addr + cur_len, read_data, needed_len);
		if (status != FLASH_OK) {
#if FLASH_BEDUG
	        printTagLog(FLASH_TAG, "flash compare addr=%08lX len=%lu error=%u (read)", addr + cur_len, needed_len, status);
#endif
	        return status;
		}

		if (memcmp(read_data, data + cur_len, needed_len)) {
			*cmp_res = true;
			break;
		}

		cur_len += needed_len;
	}

	return FLASH_OK;
}

flash_status_t _flash_read(uint32_t addr, uint8_t* data, uint32_t len)
{
    if (addr + len > _flash_get_storage_bytes_size()) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%08lX len=%lu: error (unacceptable address)", addr, len);
#endif
        return FLASH_OOM;
    }

    uint8_t spi_cmd[FLASH_SPI_COMMAND_SIZE_MAX] = { 0 };
    uint8_t counter = 0;
    spi_cmd[counter++] = FLASH_W25_CMD_READ;
    if (flash_info.is_24bit_address) {
        spi_cmd[counter++] = (addr >> 24) & 0xFF;
    }
    spi_cmd[counter++] = (addr >> 16) & 0xFF;
    spi_cmd[counter++] = (addr >> 8) & 0xFF;
    spi_cmd[counter++] = addr & 0xFF;

    flash_status_t status = FLASH_OK;
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%08lX len=%lu: error (FLASH busy)", addr, len);
#endif
        return FLASH_BUSY;
    }

    status = _flash_send_data(spi_cmd, counter);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%08lX len=%lu: error=%u (send command)", addr, len, status);
#endif
        return status;
    }

    if (data && len) {
    	status = _flash_recieve_data(data, len);
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%08lX len=%lu: error=%u (recieve data)", addr, len, status);
#endif
    }

    return status;
}

flash_status_t _flash_read_jdec_id(uint32_t* jdec_id)
{
#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "get JEDEC ID: begin");
#endif

    flash_status_t status = FLASH_BUSY;
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get JEDEC ID error (FLASH busy)");
#endif
        goto do_spi_stop;
    }

	_flash_spi_cs_set();
    uint8_t spi_cmd[] = { FLASH_W25_CMD_JEDEC_ID };
    status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get JDEC ID error=%u (send command)", status);
#endif
        goto do_spi_stop;
    }

    uint8_t data[FLASH_W25_JEDEC_ID_SIZE] = { 0 };
    status = _flash_recieve_data(data, sizeof(data));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get JDEC ID error=%u (recieve data)", status);
#endif
        goto do_spi_stop;
    }

    *jdec_id = ((((uint32_t)data[0]) << 16) | (((uint32_t)data[1]) << 8) | ((uint32_t)data[2]));

do_spi_stop:
	_flash_spi_cs_reset();

    return status;
}

flash_status_t _flash_read_SR1(uint8_t* SR1)
{
    uint8_t spi_cmd[] = { FLASH_W25_CMD_READ_SR1 };

    bool cs_enabled = !(bool)HAL_GPIO_ReadPin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin);
	if (cs_enabled) {
	    _flash_spi_cs_reset();
	}
    _flash_spi_cs_set();

    HAL_StatusTypeDef status = HAL_SPI_Transmit(&FLASH_SPI, spi_cmd, sizeof(spi_cmd), FLASH_SPI_TIMEOUT_MS);
    if (status != HAL_OK) {
        goto do_spi_stop;
    }

    status = HAL_SPI_Receive(&FLASH_SPI, SR1, sizeof(uint8_t), FLASH_SPI_TIMEOUT_MS);
    if (status != HAL_OK) {
        goto do_spi_stop;
    }

do_spi_stop:
	_flash_spi_cs_reset();
	if (cs_enabled) {
		_flash_spi_cs_set();
	}
    if (status == HAL_BUSY) {
    	return FLASH_BUSY;
    }
    if (status != HAL_OK) {
    	return FLASH_ERROR;
    }

    return FLASH_OK;
}

flash_status_t _flash_write_enable()
{
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "write enable error (FLASH busy)");
#endif
        return FLASH_BUSY;
    }

    uint8_t spi_cmd[] = { FLASH_W25_CMD_WRITE_ENABLE };
    flash_status_t status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
#if FLASH_BEDUG
    if (status != FLASH_OK) {
        printTagLog(FLASH_TAG, "write enable error=%u", status);
    }
#endif

    return status;
}

flash_status_t _flash_write_disable()
{
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "write disable error (FLASH busy)");
#endif
        return FLASH_BUSY;
    }

    uint8_t spi_cmd[] = { FLASH_W25_CMD_WRITE_DISABLE };
    flash_status_t status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
#if FLASH_BEDUG
    if (status != FLASH_OK) {
        printTagLog(FLASH_TAG, "write disable error=%u", status);
    }
#endif

    return status;
}

flash_status_t _flash_erase_sector(uint32_t addr)
{
#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "flash erase sector addr=%08lX: begin", addr);
#endif

    if (addr % flash_info.sector_size > 0) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%08lX error (unacceptable address)", addr);
#endif
        return FLASH_ERROR;
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%08lX error (flash is busy)", addr);
#endif
        return FLASH_BUSY;
    }

    uint8_t spi_cmd[FLASH_SPI_COMMAND_SIZE_MAX] = { 0 };
    uint8_t counter = 0;
    spi_cmd[counter++] = FLASH_W25_CMD_ERASE_SECTOR;
    if (flash_info.is_24bit_address) {
        spi_cmd[counter++] = (addr >> 24) & 0xFF;
    }
    spi_cmd[counter++] = (addr >> 16) & 0xFF;
    spi_cmd[counter++] = (addr >> 8) & 0xFF;
    spi_cmd[counter++] = addr & 0xFF;

    flash_status_t status = _flash_set_protect_block(FLASH_W25_SR1_UNBLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%08lX error=%u (unset block protect)", addr, status);
#endif
        goto do_spi_stop;
    }

    status = _flash_write_enable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%08lX error=%u (write is not enabled)", addr, status);
#endif
        goto do_spi_stop;
    }

    if (!util_wait_event(_flash_check_WEL, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%08lX error=%u (WEL bit wait time exceeded)", addr, FLASH_BUSY);
#endif
        status = FLASH_BUSY;
        goto do_spi_stop;
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%08lX error=%u (BUSY bit wait time exceeded)", addr, FLASH_BUSY);
#endif
        status = FLASH_BUSY;
        goto do_spi_stop;
    }

    status = _flash_send_data(spi_cmd, counter);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%08lX error=%u (write is not enabled)", addr, status);
#endif
        goto do_spi_stop;
    }

    status = _flash_write_disable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%08lX error=%u (write is not disabled)", addr, status);
#endif
        goto do_spi_stop;
    }

do_spi_stop:
    if (status != FLASH_OK) {
        goto do_block_protect;
    }

    flash_status_t tmp_status = FLASH_OK;
do_block_protect:
    tmp_status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status == FLASH_OK) {
        status = tmp_status;
    } else {
        return status;
    }

#if FLASH_BEDUG
    if (status == FLASH_OK) {
    	printTagLog(FLASH_TAG, "flash erase sector addr=%08lX: OK", addr);
    } else {
        printTagLog(FLASH_TAG, "erase sector addr=%08lX error=%u (set block protected)", addr, status);
        status = FLASH_BUSY;
    }
#endif

    return status;
}

flash_status_t _flash_set_protect_block(uint8_t value)
{
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "set protect block value=%02X error (FLASH busy)", value);
#endif
        return FLASH_BUSY;
    }

    uint8_t spi_cmd_01[] = { FLASH_W25_CMD_WRITE_ENABLE_SR };

    flash_status_t status = _flash_send_data(spi_cmd_01, sizeof(spi_cmd_01));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "set protect block value=%02X error=%u (enable write SR1)", value, status);
#endif
        goto do_spi_stop;
    }


    uint8_t spi_cmd_02[] = { FLASH_W25_CMD_WRITE_SR1, ((value & 0x0F) << 2) };

    status = _flash_send_data(spi_cmd_02, sizeof(spi_cmd_02));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "set protect block value=%02X error=%u (write SR1)", value, status);
#endif
    }

do_spi_stop:
    return status;
}


flash_status_t _flash_send_data(const uint8_t* data, const uint16_t len)
{
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&FLASH_SPI, (uint8_t*)data, (uint16_t)len, FLASH_SPI_TIMEOUT_MS);

    if (status == HAL_BUSY) {
    	return FLASH_BUSY;
    }
    if (status != HAL_OK) {
    	return FLASH_ERROR;
    }

    return FLASH_OK;
}

flash_status_t _flash_recieve_data(uint8_t* data, uint16_t len)
{
    HAL_StatusTypeDef status =  HAL_SPI_Receive(&FLASH_SPI, data, len, FLASH_SPI_TIMEOUT_MS);

    if (status == HAL_BUSY) {
    	return FLASH_BUSY;
    }
    if (status != HAL_OK) {
    	return FLASH_ERROR;
    }

    return FLASH_OK;
}

void _flash_spi_cs_set()
{
    HAL_GPIO_WritePin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin, GPIO_PIN_RESET);
}

void _flash_spi_cs_reset()
{
    HAL_GPIO_WritePin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin, GPIO_PIN_SET);
}

bool _flash_check_FREE()
{
    uint8_t SR1 = 0x00;
    flash_status_t status = _flash_read_SR1(&SR1);
    if (status != FLASH_OK) {
        return false;
    }

    return !(SR1 & FLASH_W25_SR1_BUSY);
}

bool _flash_check_WEL()
{
    uint8_t SR1 = 0x00;
    flash_status_t status = _flash_read_SR1(&SR1);
    if (status != FLASH_OK) {
        return false;
    }

    return SR1 & FLASH_W25_SR1_WEL;
}

uint32_t _flash_get_storage_bytes_size()
{
#if FLASH_TEST
	return flash_w25qxx_get_pages_count() * FLASH_W25_PAGE_SIZE;
#endif
    return flash_info.blocks_count * flash_info.block_size;
}
