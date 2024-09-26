/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <random>

#include "main.h"
#include "soul.h"
#include "system.h"
#include "hal_defs.h"

#ifdef EEPROM_MODE
#   include "at24cm01.h"
#else
#   include "w25qxx.h"
#endif


#define ERRORS_MAX (5)


MemoryWatchdog::MemoryWatchdog():
	errorTimer(TIMEOUT_MS), timer(SECOND_MS), errors(0), timerStarted(false)
	{}

void MemoryWatchdog::check()
{
	if (timer.wait()) {
		return;
	}
	timer.start();

	uint8_t data = 0;
#ifdef EEPROM_MODE
	eeprom_status_t status = EEPROM_OK;
#else
	flash_status_t status = FLASH_OK;
#endif
	if (is_status(MEMORY_READ_FAULT) ||
		is_status(MEMORY_WRITE_FAULT) ||
		is_error(MEMORY_ERROR)
	) {
#ifdef EEPROM_MODE
		system_reset_i2c_errata();

		uint32_t address = static_cast<uint32_t>(rand()) % eeprom_get_size();

		status = eeprom_read(address, &data, sizeof(data));
		if (status == EEPROM_OK) {
			reset_status(MEMORY_READ_FAULT);
			status = eeprom_write(address, &data, sizeof(data));
		} else {
			errors++;
		}
		if (status == EEPROM_OK) {
			reset_status(MEMORY_WRITE_FAULT);
			timerStarted = false;
			errors = 0;
		} else {
			errors++;
		}
#else
		uint32_t address = static_cast<uint32_t>(rand()) % (flash_w25qxx_get_pages_count() * FLASH_W25_PAGE_SIZE);

		status = flash_w25qxx_read(address, &data, sizeof(data));
		if (status == FLASH_OK) {
			reset_status(MEMORY_READ_FAULT);
			status = flash_w25qxx_write(address, &data, sizeof(data));
		} else {
			errors++;
		}
		if (status == FLASH_OK) {
			reset_status(MEMORY_WRITE_FAULT);
			timerStarted = false;
			errors = 0;
		} else {
			errors++;
		}
#endif
	}

	(errors > ERRORS_MAX) ? set_error(MEMORY_ERROR) : reset_error(MEMORY_ERROR);

	if (!timerStarted && is_error(MEMORY_ERROR)) {
		timerStarted = true;
		errorTimer.start();
	}

	if (timerStarted && !errorTimer.wait()) {
		system_error_handler(MEMORY_ERROR, NULL);
	}
}
