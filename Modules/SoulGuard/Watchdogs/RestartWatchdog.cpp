/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "glog.h"
#include "main.h"
#include "system.h"
#include "hal_defs.h"

#include "CodeStopwatch.h"


bool RestartWatchdog::flagsCleared = false;


void RestartWatchdog::check()
{
#if WATCHDOG_BEDUG
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);
#endif

	if (flagsCleared) {
		return;
	}

	bool flag = false;
	// IWDG check reboot
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
#if WATCHDOG_BEDUG
		printTagLog(TAG, "IWDG just went off");
#endif
		flag = true;
	}

	// WWDG check reboot
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
#if WATCHDOG_BEDUG
		printTagLog(TAG, "WWDG just went off");
#endif
		flag = true;
	}

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
#if WATCHDOG_BEDUG
		printTagLog(TAG, "SOFT RESET");
#endif
		flag = true;
	}

	if (flag) {
		__HAL_RCC_CLEAR_RESET_FLAGS();
#if WATCHDOG_BEDUG
		printTagLog(TAG, "DEVICE HAS BEEN REBOOTED");
#endif
		system_reset_i2c_errata();
		HAL_Delay(2500);
	}
}
