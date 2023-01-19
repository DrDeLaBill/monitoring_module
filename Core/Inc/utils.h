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


#ifndef __sgn
#define __sgn(v) (((v) > 0) ? 1 : -1)
#endif


typedef struct _dio_timer_t {
	uint32_t start;
	uint32_t delay;
} dio_timer_t;

void Util_TimerStart(dio_timer_t* tm, uint32_t waitMs);
uint8_t Util_TimerPending(dio_timer_t* tm);


void Debug_HexDump(const char* tag, const uint8_t* buf, uint16_t len);


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


#define SUBTRACT_DELTA(var, delta) { var -= (var <= delta) ? var : delta; }


#define FLOAT_AS_STRINGS(fl_num) (int)fl_num, (int)(fl_num * 100) % 100


#endif /* INC_UTILS_H_ */
