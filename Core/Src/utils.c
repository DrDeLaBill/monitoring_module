/*
 * utils.c
 *
 *  Created on: May 20, 2022
 *      Author: gauss
 */

#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "utils.h"
#include "defines.h"


void util_timer_start(dio_timer_t* tm, uint32_t waitMs) {
	tm->start = HAL_GetTick();
	tm->delay = waitMs;
}


uint8_t util_is_timer_wait(dio_timer_t* tm) {
	return (HAL_GetTick() - tm->start) < tm->delay;
}

int util_convert_range(int val, int rngl1, int rngh1, int rngl2, int rngh2) {
	int range1 = __abs(rngh1 - rngl1);
	int range2 = __abs(rngh2 - rngl2);
	int delta  = __abs(rngh1 - val);
	return rngl2 + ((delta * range2) / range1);
}

uint16_t util_get_crc16(uint8_t* buf, uint16_t len) {
	uint16_t crc;
	for (uint16_t i = 1; i < len; i++) {
		crc = (uint8_t)(crc >> 8) | (crc << 8);
		crc ^= buf[i];
		crc ^= (uint8_t)(crc & 0xFF) >> 4;
		crc ^= (crc << 8) << 4;
		crc ^= ((crc & 0xff) << 4) << 1;
	}
	return crc;
}

#ifdef DEBUG
void util_debug_hex_dump(const char* tag, const uint8_t* buf, uint16_t len) {
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
void util_debug_hex_dump(const char* tag, const uint8_t* buf, uint16_t len) {}
#endif /* DEBUG */
