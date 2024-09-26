/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "system.h"
#include "hal_defs.h"

#include "CodeStopwatch.h"


void PowerWatchdog::check()
{
#if WATCHDOG_BEDUG
	utl::CodeStopwatch stopwatch("PWRw", WATCHDOG_TIMEOUT_MS);
#endif

	if (!is_status(WORKING)) {
		return;
	}

	uint32_t voltage = get_system_power();

	if (STM_MIN_VOLTAGEx10 <= voltage && voltage <= STM_MAX_VOLTAGEx10) {
		reset_error(POWER_ERROR);
	} else {
		set_error(POWER_ERROR);
	}
}
