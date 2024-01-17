/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include "main.h"
#include "defines.h"

#include "StoragePage.h"


#define SETTINGS_BEDUG (true)


class SettingsDB
{
public:
    typedef enum _SettingsStatus {
        SETTINGS_OK = 0,
        SETTINGS_ERROR
    } SettingsStatus;

    SettingsDB();

    SettingsStatus load();
    SettingsStatus save();
    SettingsStatus reset();

    void set_id(uint32_t id);
    void set_cf_id(uint32_t cf_id);

    bool isLoaded();
    void show();

    // TODO: server id get from server
    // TODO: pump speed must be recalculate by liquid level, after receive from server
    typedef struct __attribute__((packed)) _Settings  {
    	uint32_t id;
    	char server_url[CHAR_SETIINGS_SIZE];
    	char server_port[CHAR_SETIINGS_SIZE];
    	// Configuration version
    	uint32_t cf_id;
    	// Enable pump
    	uint8_t pump_enabled;
    	// Measure delay in milliseconds
    	uint32_t sleep_time;
    	// Current server log ID
    	uint32_t server_log_id;
    	// Liters ADC value when liquid tank can be considered full
    	uint32_t tank_ADC_min;
    	// Liters ADC value when liquid tank can be considered empty
    	uint32_t tank_ADC_max;
    	// Liters value when liquid tank can be considered full
    	uint32_t tank_liters_max;
    	// Liters value when liquid tank can be considered empty
    	uint32_t tank_liters_min;
    	// Target milliliters per day for pump
    	uint32_t pump_target;
    	// Pump speed: milliliters per hour
    	uint32_t pump_speed;
    	// Current pump work sec
    	uint32_t pump_work_sec;
    	// Current day pump downtime sec
    	uint32_t pump_downtime_sec;
    	// Pump work sec for current day
    	uint32_t pump_work_day_sec;
    	// Current log day
    	uint8_t pump_log_date;
    	// First start flag
    	uint8_t is_first_start;
    } Settings;

	static const char defaultUrl[CHAR_SETIINGS_SIZE];
    static const char defaultPort[CHAR_SETIINGS_SIZE];

    Settings settings;

    typedef struct _DeviceInfo {
        bool settings_loaded;
        bool saved_new_log;
    } DeviceInfo;

    DeviceInfo info;

private:
    static const char* SETTINGS_PREFIX;
    static const char* TAG;

    static const uint8_t VERSION = ((uint8_t)0x01);
    static const uint8_t DEVICE_ID_SIZE  = ((uint8_t)16);
    static const uint32_t DEFAULT_ID     = ((uint32_t)1);

    bool check(Settings* settings);
};
