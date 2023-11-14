/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "utils.h"


#define LOG_BEDUG (true)


class LogService
{
private:
	static constexpr char TAG[]             = "LOG";

	static constexpr char T_DASH_FIELD[]    = "-";
	static constexpr char T_TIME_FIELD[]    = "t";
	static constexpr char T_COLON_FIELD[]   = ":";

	static constexpr char TIME_FIELD[]      = "t";
	static constexpr char CF_ID_FIELD[]     = "cf_id";
	static constexpr char CF_DATA_FIELD[]   = "cf";
	static constexpr char CF_DEV_ID_FIELD[] = "id";
	static constexpr char CF_PWR_FIELD[]    = "pwr";
	static constexpr char CF_LTRMIN_FIELD[] = "ltrmin";
	static constexpr char CF_LTRMAX_FIELD[] = "ltrmax";
	static constexpr char CF_TRGT_FIELD[]   = "trgt";
	static constexpr char CF_SLEEP_FIELD[]  = "sleep";
	static constexpr char CF_SPEED_FIELD[]  = "speed";
	static constexpr char CF_LOGID_FIELD[]  = "d_hwm";
	static constexpr char CF_CLEAR_FIELD[]  = "clr";

	static constexpr uint32_t LOG_SIZE = 370;

	static util_timer_t logTimer;
	static util_timer_t settingsTimer;

	static uint32_t logId;

	static constexpr uint32_t settingsDelayMs = 60000;

	static void sendRequest();
	static void parse();
	static bool saveNewLog();
	static bool findParam(char** dst, const char* src, const char* param);
	static bool updateTime(char* data);
	static void clearLog();
	static void saveResponse();

public:
	static void update();

};
