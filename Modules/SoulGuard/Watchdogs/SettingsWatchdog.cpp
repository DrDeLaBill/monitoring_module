/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <cstring>

#include "glog.h"
#include "soul.h"
#include "main.h"
#include "settings.h"

#include "SettingsDB.h"
#include "CodeStopwatch.h"


fsm::FiniteStateMachine<SettingsWatchdog::fsm_table> SettingsWatchdog::fsm;


SettingsWatchdog::SettingsWatchdog()
{
	set_status(LOADING);
}

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
		if (!settings_check(&settings)) {
			status = SETTINGS_ERROR;
		}
	}

	if (status != SETTINGS_OK) {
		settings_repair(&settings);
		status = settingsDB.save();
		if (status == SETTINGS_OK) {
#if SETTINGS_WATCHDOG_BEDUG
			printTagLog(TAG, "state_init: event_saved");
#endif
		}
	}

	if (status == SETTINGS_OK) {
		reset_error(SETTINGS_LOAD_ERROR);
		settings_show();

		set_status(SETTINGS_INITIALIZED);
		reset_status(LOADING);

		fsm.push_event(updated_e{});
	} else {
		set_error(SETTINGS_LOAD_ERROR);
	}
}

void SettingsWatchdog::state_idle::operator ()() const
{
	if (is_status(NEED_SAVE_SETTINGS)) {
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "state_idle: event_updated");
#endif
		set_status(LOADING);
		fsm.push_event(updated_e{});
	} else if (is_status(NEED_LOAD_SETTINGS)) {
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "state_idle: event_saved");
#endif
		set_status(LOADING);
		fsm.push_event(saved_e{});
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
		fsm.push_event(saved_e{});
		settings_show();

		reset_error(SETTINGS_LOAD_ERROR);

		reset_status(NEED_SAVE_SETTINGS);
		reset_status(LOADING);
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
		fsm.push_event(updated_e{});
		settings_show();

		reset_error(SETTINGS_LOAD_ERROR);
		reset_status(NEED_LOAD_SETTINGS);
		reset_status(LOADING);
	}
}

void SettingsWatchdog::action_check::operator ()() const
{
	reset_error(SETTINGS_LOAD_ERROR);
	if (!settings_check(&settings)) {
		set_error(SETTINGS_LOAD_ERROR);
#if SETTINGS_WATCHDOG_BEDUG
		printTagLog(TAG, "action_check: event_not_valid");
#endif
		settings_repair(&settings);
		set_status(NEED_SAVE_SETTINGS);
	}
}
