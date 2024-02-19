/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


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
