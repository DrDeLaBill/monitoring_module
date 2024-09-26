/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <cstring>

#include "glog.h"
#include "soul.h"
#include "main.h"
#include "fsm_gc.h"
#include "settings.h"

#include "Timer.h"
#include "SettingsDB.h"
#include "CodeStopwatch.h"


void _stng_check(void);

void _stng_init_s(void);
void _stng_idle_s(void);
void _stng_save_s(void);
void _stng_load_s(void);

void _stng_update_hash_a(void);


#if WATCHDOG_BEDUG
const char STNGw_TAG[] = "STGw";
#endif


static unsigned old_hash = 0;
static unsigned new_hash = 0;


FSM_GC_CREATE(stng_fsm)

FSM_GC_CREATE_EVENT(stng_saved_e,   0)
FSM_GC_CREATE_EVENT(stng_updated_e, 0)

FSM_GC_CREATE_STATE(stng_init_s, _stng_init_s)
FSM_GC_CREATE_STATE(stng_idle_s, _stng_idle_s)
FSM_GC_CREATE_STATE(stng_save_s, _stng_save_s)
FSM_GC_CREATE_STATE(stng_load_s, _stng_load_s)

FSM_GC_CREATE_TABLE(
	stng_fsm_table,
	{&stng_init_s, &stng_updated_e, &stng_idle_s, _stng_update_hash_a},

	{&stng_idle_s, &stng_saved_e,   &stng_load_s, _stng_update_hash_a},
	{&stng_idle_s, &stng_updated_e, &stng_save_s, _stng_update_hash_a},

	{&stng_load_s, &stng_updated_e, &stng_idle_s, _stng_update_hash_a},
	{&stng_save_s, &stng_saved_e,   &stng_idle_s, _stng_update_hash_a}
)


SettingsWatchdog::SettingsWatchdog()
{
	set_status(LOADING);

	fsm_gc_init(&stng_fsm, stng_fsm_table, __arr_len(stng_fsm_table));
}

void SettingsWatchdog::check()
{
#if WATCHDOG_BEDUG
	utl::CodeStopwatch stopwatch("STNG", GENERAL_TIMEOUT_MS);
#endif

	fsm_gc_proccess(&stng_fsm);
}

void _stng_check(void)
{
	reset_error(SETTINGS_LOAD_ERROR);
	if (!settings_check(&settings)) {
		set_error(SETTINGS_LOAD_ERROR);
#if WATCHDOG_BEDUG
		printTagLog(STNGw_TAG, "settings check: event_not_valid");
#endif
		settings_repair(&settings);
		set_status(NEED_SAVE_SETTINGS);
	}
}

void _stng_init_s(void)
{
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&settings), settings_size());
	SettingsStatus status = settingsDB.load();
	if (status == SETTINGS_OK) {
#if WATCHDOG_BEDUG
		printTagLog(STNGw_TAG, "state_init: event_loaded");
#endif
		if (!settings_check(&settings)) {
			status = SETTINGS_ERROR;
		}
	}

	if (status != SETTINGS_OK) {
		settings_repair(&settings);
		status = settingsDB.save();
		if (status == SETTINGS_OK) {
#if WATCHDOG_BEDUG
			printTagLog(STNGw_TAG, "state_init: event_saved");
#endif
		}
	}

	if (status == SETTINGS_OK) {
		reset_error(SETTINGS_LOAD_ERROR);
		settings_show();

		set_status(SETTINGS_INITIALIZED);
		reset_status(LOADING);

		_stng_check();
		fsm_gc_push_event(&stng_fsm, &stng_updated_e);
	} else {
		set_error(SETTINGS_LOAD_ERROR);
	}
}

void _stng_idle_s(void)
{
	if (is_status(NEED_SAVE_SETTINGS)) {
#if WATCHDOG_BEDUG
		printTagLog(STNGw_TAG, "state_idle: event_updated");
#endif
		set_status(LOADING);
		_stng_check();
		fsm_gc_push_event(&stng_fsm, &stng_updated_e);
	} else if (is_status(NEED_LOAD_SETTINGS)) {
#if WATCHDOG_BEDUG
		printTagLog(STNGw_TAG, "state_idle: event_saved");
#endif
		set_status(LOADING);
		_stng_check();
		fsm_gc_push_event(&stng_fsm, &stng_saved_e);
	}
}

void _stng_save_s(void)
{
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&settings), settings_size());
	SettingsStatus status = settingsDB.save();
	if (status == SETTINGS_OK) {
#if WATCHDOG_BEDUG
		printTagLog(STNGw_TAG, "state_save: event_saved");
#endif
		_stng_check();
		fsm_gc_push_event(&stng_fsm, &stng_saved_e);

		settings_show();

		reset_error(SETTINGS_LOAD_ERROR);

		reset_status(NEED_SAVE_SETTINGS);
		reset_status(LOADING);
	}
}

void _stng_load_s(void)
{
	SettingsDB settingsDB(reinterpret_cast<uint8_t*>(&settings), settings_size());
	SettingsStatus status = settingsDB.load();
	if (status == SETTINGS_OK) {
#if WATCHDOG_BEDUG
		printTagLog(STNGw_TAG, "state_load: event_loaded");
#endif
		_stng_check();
		fsm_gc_push_event(&stng_fsm, &stng_updated_e);

		settings_show();

		reset_error(SETTINGS_LOAD_ERROR);
		reset_status(NEED_LOAD_SETTINGS);
		reset_status(LOADING);
	}
}

void _stng_update_hash_a(void)
{
	old_hash = util_hash((uint8_t*)&settings, sizeof(settings));
	new_hash = old_hash;
}
