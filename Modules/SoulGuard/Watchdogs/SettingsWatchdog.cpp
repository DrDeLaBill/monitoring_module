/* Copyright © 2024 Georgy E. All rights reserved. */

#include "SettingsWatchdog.h"

#include <cstring>

#include "log.h"
#include "soul.h"
#include "main.h"
#include "settings.h"

#include "SettingsDB.h"
#include "CodeStopwatch.h"


fsm::FiniteStateMachine<SettingsWatchdog::fsm_table> SettingsWatchdog::fsm;


SettingsWatchdog::SettingsWatchdog() { }

void SettingsWatchdog::check()
{
	utl::CodeStopwatch stopwatch("STNG", GENERAL_TIMEOUT_MS);
	fsm.proccess();
}

void SettingsWatchdog::state_init::operator ()() const
{
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&settings), settings_size());
	SettingsStatus status = settingsDB.load();
	if (status == SETTINGS_OK) {
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "state_init: event_loaded");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_loaded{});
		reset_error(SETTINGS_ERROR);
	    set_settings_initialized();
		settings_show();
		return;
	}

	settings_reset(&settings);
	status = settingsDB.save();
	if (status == SETTINGS_OK) {
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "state_init: event_saved");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_saved{});
		reset_error(SETTINGS_ERROR);
	    set_settings_initialized();
		settings_show();
		return;
	}

	set_error(SETTINGS_ERROR);
}

void SettingsWatchdog::state_idle::operator ()() const
{
	if (is_settings_updated()) {
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "state_idle: event_updated");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_updated{});
	} else if (is_settings_saved()) {
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "state_idle: event_saved");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_saved{});
	}
}

void SettingsWatchdog::state_save::operator ()() const
{
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&settings), settings_size());
	SettingsStatus status = settingsDB.save();
	if (status == SETTINGS_OK) {
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "state_save: event_saved");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_saved{});
		set_settings_update_status(false);
		settings_show();
	}
}

void SettingsWatchdog::state_load::operator ()() const
{
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&settings), settings_size());
	SettingsStatus status = settingsDB.load();
	if (status == SETTINGS_OK) {
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "state_load: event_loaded");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_loaded{});
		set_settings_save_status(false);
		settings_show();
	}
}

void SettingsWatchdog::action_check::operator ()() const
{
	if (!settings_check(&settings)) {
		set_error(SETTINGS_ERROR);
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "action_check: event_not_valid");
#endif
		SettingsWatchdog::fsm.push_event(SettingsWatchdog::event_not_valid{});
	}
}

void SettingsWatchdog::action_reset::operator ()() const
{
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "action_reset: reset");
#endif
	settings_reset(&settings);
	set_settings_update_status(true);
}
