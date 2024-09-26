/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <cstring>

#include "glog.h"
#include "main.h"
#include "soul.h"
#include "main.h"
#include "gutils.h"
#include "bmacro.h"

#include "CodeStopwatch.h"


#define STACK_CANARY_WORD ((uint32_t)0xBEDAC0DE)
#define STACK_PERCENT_MIN (5)


unsigned StackWatchdog::lastFree = 0;
utl::Timer StackWatchdog::timer(WATCHDOG_TIMEOUT_MS);


void STACK_WATCHDOG_FILL_RAM(void) {
	extern unsigned _ebss;
	volatile unsigned *top, *start;
	__asm__ volatile ("mov %[top], sp" : [top] "=r" (top) : : );
	start = &_ebss;
	while (start < top) {
		*(start++) = STACK_CANARY_WORD;
	}
}


void StackWatchdog::check()
{
#if WATCHDOG_BEDUG
	utl::CodeStopwatch stopwatch("STCK", WATCHDOG_TIMEOUT_MS);
#endif

	if (timer.wait()) {
		return;
	}
	timer.start();

	extern unsigned _ebss;
	unsigned *start, *end;
	__asm__ volatile ("mov %[end], sp" : [end] "=r" (end) : : );
	start = &_ebss;

	unsigned heap_end = 0;
	unsigned stack_end = 0;
	unsigned last_counter = 0;
	unsigned cur_counter = 0;
	for (;start < end; start++) {
		if ((*start) == STACK_CANARY_WORD) {
			cur_counter++;
		}
		if (cur_counter && (*start) != STACK_CANARY_WORD) {
			if (last_counter < cur_counter) {
				last_counter = cur_counter;
				heap_end     = (unsigned)start - cur_counter;
				stack_end    = (unsigned)start;
			}

			cur_counter = 0;
		}
	}

	extern unsigned _sdata;
	extern unsigned _estack;
	uint32_t freeRamBytes = last_counter * sizeof(STACK_CANARY_WORD);
	unsigned freePercent = (unsigned)__percent(
		(uint32_t)last_counter,
		(uint32_t)__abs_dif(&_sdata, &_estack)
	);
#if WATCHDOG_BEDUG
	if (freeRamBytes && __abs_dif(lastFree, freeRamBytes)) {
		printTagLog(TAG, "-----ATTENTION! INDIRECT DATA BEGIN:-----");
		printTagLog(TAG, "RAM:              [0x%08X->0x%08X]", (unsigned)&_sdata, (unsigned)&_estack);
		printTagLog(TAG, "RAM occupied MAX: %u bytes", (unsigned)(__abs_dif((unsigned)&_sdata, (unsigned)&_estack) - freeRamBytes));
		printTagLog(TAG, "RAM free  MIN:    %u bytes (%u%%) [0x%08X->0x%08X]", (unsigned)freeRamBytes, freePercent, (unsigned)(stack_end - freeRamBytes), (unsigned)stack_end);
		printTagLog(TAG, "------ATTENTION! INDIRECT DATA END-------");
	}
#endif

	if (freeRamBytes) {
		lastFree = freeRamBytes;
	}

	if (freeRamBytes && lastFree && heap_end < stack_end && freePercent > STACK_PERCENT_MIN) {
		reset_error(STACK_ERROR);
	} else {
		set_error(STACK_ERROR);
#if WATCHDOG_BEDUG
		BEDUG_ASSERT(
			false,
			"STACK OVERFLOW IS POSSIBLE or the function STACK_WATCHDOG_FILL_RAM was not used on startup"
		);
#endif
	}


}
