/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "glog.h"
#include "main.h"
#include "soul.h"
#include "clock.h"
#include "bmacro.h"
#include "system.h"
#include "settings.h"

#include "Timer.h"
#include "CodeStopwatch.h"


void RTCWatchdog::check()
{
#if WATCHDOG_BEDUG
	utl::CodeStopwatch stopwatch("RTC", GENERAL_TIMEOUT_MS);
#endif

	static bool initialized = false;
	if (!initialized) {
		if (is_clock_ready()) {
			initialized = true;
			set_status(DS1307_READY);
		} else {
			reset_status(DS1307_READY);
		}
		return;
	}

	static utl::Timer timer(10 * SECOND_MS);
	if (timer.wait()) {
		return;
	}
	timer.start();

	clock_date_t date = {};
	clock_time_t time = {};

	if (!clock_get_rtc_date(&date)) {
		system_reset_i2c_errata();
		reset_status(DS1307_READY);
		set_error(RTC_ERROR);
		return;
	} else {
		reset_error(RTC_ERROR);
	}

	if (!clock_get_rtc_time(&time)) {
		system_reset_i2c_errata();
		reset_status(DS1307_READY);
		set_error(RTC_ERROR);
		return;
	} else {
		reset_error(RTC_ERROR);
	}

	bool updateFlag = false;
	if (date.Date == 0 || date.Date > 31) {
#if WATCHDOG_BEDUG
		printTagLog(TAG, "WARNING! The date of the clock has been reset to 1");
#endif
		updateFlag = true;
	}
	if (date.Month == 0 || date.Month > 12) {
#if WATCHDOG_BEDUG
		printTagLog(TAG, "WARNING! The month of the clock has been reset to 1");
#endif
		updateFlag = true;
	}

	if (updateFlag) {
		if (!clock_save_date(&date)) {
			system_reset_i2c_errata();
			reset_status(DS1307_READY);
			set_error(RTC_ERROR);
			return;
		} else {
			reset_error(RTC_ERROR);
		}
	}

	updateFlag = false;
	if (time.Seconds > 59) {
#if WATCHDOG_BEDUG
		printTagLog(TAG, "WARNING! The seconds of the clock has been reset to 1");
#endif
		updateFlag = true;
		time.Seconds = 0;
	}
	if (time.Minutes > 59) {
#if WATCHDOG_BEDUG
		printTagLog(TAG, "WARNING! The minutes of the clock has been reset to 1");
#endif
		updateFlag = true;
		time.Minutes = 0;
	}
	if (time.Hours > 23) {
#if WATCHDOG_BEDUG
		printTagLog(TAG, "WARNING! The hours of the clock has been reset to 1");
#endif
		updateFlag = true;
		time.Hours = 0;
	}

	if (updateFlag) {
		if (!clock_save_time(&time)) {
			system_reset_i2c_errata();
			reset_status(DS1307_READY);
			set_error(RTC_ERROR);
			return;
		} else {
			reset_error(RTC_ERROR);
		}
	}
}
