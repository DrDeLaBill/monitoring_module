/*
 * utils.c
 *
 *  Created on: May 20, 2022
 *      Author: gauss
 */

#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "utils.h"


void Util_TimerStart(dio_timer_t* tm, uint32_t waitMs) {
	tm->start = HAL_GetTick();
	tm->delay = waitMs;
}


uint8_t Util_TimerPending(dio_timer_t* tm) {
	return (HAL_GetTick() - tm->start) < tm->delay;
}


#ifdef DEBUG
void Debug_HexDump(const char* tag, const uint8_t* buf, uint16_t len) {
	const uint8_t ncols = 16;
	uint8_t will_print = 0, was_printed = 1;
	uint8_t not_first_line = 0;
	uint16_t pos = 0;
	while(len) {
		if(pos % ncols == 0) {
			will_print = 0;
			for(uint8_t i = 0; i < __min(ncols, len); i ++) will_print |= buf[i];
			if(will_print) {
				if(not_first_line) LOG_DEBUG_LN("\n");
				LOG_DEBUG(tag, "[%04X] ", pos);

			} else if(was_printed) {
				if(not_first_line) LOG_DEBUG_LN("\n");
				LOG_DEBUG(tag, "[%04X] ...", pos);

				was_printed = 0;
			}
			not_first_line = 1;
		}
		if(will_print) {
			was_printed = 1; LOG_DEBUG_LN("%02X ", (*buf));
		}
		buf ++; len --; pos ++;
	}
	LOG_DEBUG_LN("\n");
}
#else /* DEBUG */
void Debug_HexDump(const char* tag, const uint8_t* buf, uint16_t len) {}
#endif /* DEBUG */
