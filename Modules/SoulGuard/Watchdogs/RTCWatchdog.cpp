/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "log.h"
#include "main.h"
#include "soul.h"
#include "clock.h"
#include "bmacro.h"
#include "settings.h"

#include "CodeStopwatch.h"


extern settings_t settings;


void RTCWatchdog::check()
{
	utl::CodeStopwatch stopwatch("RTC", GENERAL_TIMEOUT_MS);

	DateTypeDef date = {};
	TimeTypeDef time = {};

	clock_get_rtc_date(&date);

	clock_get_rtc_time(&time);

	bool updateFlag = false;
	if (date.Date == 0 || date.Date > 31) {
		printTagLog(TAG, "WARNING! The date of the clock has been reset to 1");
		updateFlag = true;
	}
	if (date.Month == 0 || date.Month > 12) {
		printTagLog(TAG, "WARNING! The month of the clock has been reset to 1");
		updateFlag = true;
	}

	if (updateFlag) {
		clock_save_date(&date);
	}

	updateFlag = false;
	if (time.Seconds > 59) {
		printTagLog(TAG, "WARNING! The seconds of the clock has been reset to 1");
		updateFlag = true;
		time.Seconds = 0;
	}
	if (time.Minutes > 59) {
		printTagLog(TAG, "WARNING! The minutes of the clock has been reset to 1");
		updateFlag = true;
		time.Minutes = 0;
	}
	if (time.Hours > 23) {
		printTagLog(TAG, "WARNING! The hours of the clock has been reset to 1");
		updateFlag = true;
		time.Hours = 0;
	}

	if (updateFlag) {
		clock_save_time(&time);
	}
}
