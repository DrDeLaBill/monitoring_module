/*
 * utils.h
 *
 *  Created on: May 20, 2022
 *      Author: gauss
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_


#include <stdio.h>


#ifndef __min
#define __min(x, y) ((x) < (y) ? (x) : (y))
#endif


#ifndef __max
#define __max(x, y) ((x) > (y) ? (x) : (y))
#endif


#ifndef __abs
#define __abs(v) (((v) < 0) ? (-(v)) : (v))
#endif


#ifndef __abs_dif
#define __abs_dif(f, s) (((f) > (s)) ? ((f) - (s)) : ((s) - (f)))
#endif


#ifndef __sgn
#define __sgn(v) (((v) > 0) ? 1 : -1)
#endif


typedef struct _dio_timer_t {
	uint32_t start;
	uint32_t delay;
} dio_timer_t;

void util_timer_start(dio_timer_t* tm, uint32_t waitMs);
uint8_t util_is_timer_wait(dio_timer_t* tm);


void util_debug_hex_dump(const char* tag, const uint8_t* buf, uint16_t len);
int util_convert_range(int val, int rngl1, int rngh1, int rngl2, int rngh2);
uint16_t util_get_crc16(uint8_t* buf, uint16_t len);


#ifdef DEBUG
#define LOG_DEBUG(MODULE_TAG, format, ...) { \
	printf("%s:", MODULE_TAG); printf(format __VA_OPT__(,) __VA_ARGS__); \
}
#define LOG_DEBUG_LN(format, ...) { \
	printf(format __VA_OPT__(,) __VA_ARGS__);   \
}
#else /* DEBUG */
#define LOG_DEBUG(MODULE_TAG, format, ...) {}
#define LOG_DEBUG_LN(format, ...) {}
#endif /* DEBUG */
#define LOG_MESSAGE(MODULE_TAG, format, ...) { \
	printf("%s:", MODULE_TAG); printf(format __VA_OPT__(,) __VA_ARGS__); \
}

#define SUBTRACT_DELTA(var, delta) { var -= (var <= delta) ? var : delta; }


#endif /* INC_UTILS_H_ */
