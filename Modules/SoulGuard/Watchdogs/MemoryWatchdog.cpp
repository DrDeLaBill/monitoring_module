/* Copyright © 2024 Georgy E. All rights reserved. */

#include "MemoryWatchdog.h"

#include "main.h"

#include "CodeStopwatch.h"


void MemoryWatchdog::check()
{
	utl::CodeStopwatch stopwatch("MEM", GENERAL_TIMEOUT_MS);
	// TODO: FLASH memory check (use ERRATA if needed)
}
