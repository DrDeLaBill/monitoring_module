#ifndef SETTINGS_DB_H
#define SETTINGS_DB_H


#include <cstdint>

#include "main.h"
#include "settings.h"


#ifdef DEBUG
#   define SETTINGS_DB_BEDUG (0)
#endif


class SettingsDB
{
private:
	const uint32_t size;
    uint8_t* settings;

    static constexpr char PREFIX[] = "STG";
    static constexpr char TAG[] = "STG";

public:
    SettingsDB(uint8_t* settings, uint32_t size);

    SettingsStatus load();
    SettingsStatus save();
};


#endif
