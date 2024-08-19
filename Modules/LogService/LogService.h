/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <memory>
#include <stdint.h>

#include "gutils.h"

#include "RecordDB.h"


#ifdef DEBUG
#   define LOG_SERVICE_BEDUG (1)
#endif


class LogService
{
private:
	static const char* TAG;

	static const char* T_DASH_FIELD;
	static const char* T_TIME_FIELD;
	static const char* T_COLON_FIELD;

	static const char* TIME_FIELD;
	static const char* CF_ID_FIELD;
	static const char* CF_DATA_FIELD;
	static const char* CF_DEV_ID_FIELD;
	static const char* CF_PWR_FIELD;
	static const char* CF_LTRMIN_FIELD;
	static const char* CF_LTRMAX_FIELD;
	static const char* CF_TRGT_FIELD;
	static const char* CF_SLEEP_FIELD;
	static const char* CF_SPEED_FIELD;
	static const char* CF_LOGID_FIELD;
	static const char* CF_CLEAR_FIELD;
	static const char* CF_URL_FIELD;

	static constexpr uint32_t LOG_SIZE = 200;

	static util_old_timer_t logTimer;
	static util_old_timer_t settingsTimer;

	static uint32_t logId;

	static std::unique_ptr<RecordDB> nextRecord;
	static bool newRecordLoaded;

	static constexpr uint32_t settingsDelayMs = 60000;

	static void sendRequest();
	static void parse();
	static void saveNewLog();
	static bool findParam(char** dst, const char* src, const char* param);
	static bool updateTime(char* data);
	static void clearLog();
	static void saveResponse();

public:
	static void update();
	static void clear() {} // TODO

	static void updateSleep(uint32_t time);

};
