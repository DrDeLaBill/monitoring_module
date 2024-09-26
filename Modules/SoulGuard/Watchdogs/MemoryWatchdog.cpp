/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <random>

#include "main.h"
#include "soul.h"
#include "system.h"
#include "hal_defs.h"
#include "at24cm01.h"


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
	eeprom_status_t status = EEPROM_OK;
	if (is_status(MEMORY_READ_FAULT) ||
		is_status(MEMORY_WRITE_FAULT) ||
		is_error(MEMORY_ERROR)
	) {
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
