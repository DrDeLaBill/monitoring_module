/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include "FiniteStateMachine.h"


#define SETTINGS_WATCHDOG_BEDUG (true)
#define POWER_WATCHDOG_BEDUG    (true)


/*
 * Filling an empty area of RAM with the STACK_CANARY_WORD value
 * For calculating the RAM fill factor
 */
extern "C" void STACK_WATCHDOG_FILL_RAM(void);


struct StackWatchdog
{
	void check();

private:
	static constexpr char TAG[] = "STCK";
	static unsigned lastFree;
};

struct RestartWatchdog
{
public:
	// TODO: check IWDG or another reboot
	void check();

	static void reset_i2c_errata();

private:
	static constexpr char TAG[] = "RSTw";
	static bool flagsCleared;

};

struct RTCWatchdog
{
	static constexpr char TAG[] = "RTCw";

	void check();
};


struct SettingsWatchdog
{
protected:
	struct state_init   {void operator()(void) const;};
	struct state_idle   {void operator()(void) const;};
	struct state_save   {void operator()(void) const;};
	struct state_load   {void operator()(void) const;};

	struct action_check {void operator()(void) const;};

	FSM_CREATE_STATE(init_s, state_init);
	FSM_CREATE_STATE(idle_s, state_idle);
	FSM_CREATE_STATE(save_s, state_save);
	FSM_CREATE_STATE(load_s, state_load);

	FSM_CREATE_EVENT(saved_e,   0);
	FSM_CREATE_EVENT(updated_e, 0);

	using fsm_table = fsm::TransitionTable<
		fsm::Transition<init_s, updated_e,   idle_s, action_check, fsm::Guard::NO_GUARD>,

		fsm::Transition<idle_s, saved_e,     load_s, action_check, fsm::Guard::NO_GUARD>,
		fsm::Transition<idle_s, updated_e,   save_s, action_check, fsm::Guard::NO_GUARD>,

		fsm::Transition<load_s, updated_e,   idle_s, action_check, fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s, saved_e,     idle_s, action_check, fsm::Guard::NO_GUARD>
	>;

	static fsm::FiniteStateMachine<fsm_table> fsm;

private:
	static constexpr char TAG[] = "STGw";

public:
	SettingsWatchdog();

	void check();

};

struct MemoryWatchdog
{
	void check();
};
