#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


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


#ifndef __arr_len
#define __arr_len(arr) (sizeof(arr) / sizeof(*arr))
#endif


typedef struct _util_timer_t {
    uint32_t start;
    uint32_t delay;
} util_timer_t;


void     util_timer_start(util_timer_t* tm, uint32_t waitMs);
bool     util_is_timer_wait(util_timer_t* tm);


void     util_debug_hex_dump(const char* tag, const uint8_t* buf, uint32_t start_counter, uint16_t len);
int      util_convert_range(int val, int rngl1, int rngh1, int rngl2, int rngh2);
uint16_t util_get_crc16(uint8_t* buf, uint16_t len);
bool     util_wait_event(bool (*condition) (void), uint32_t time);


#ifdef DEBUG

#define LOG_TAG_BEDUG(MODULE_TAG, format, ...) if (strlen(MODULE_TAG)) { printf("%s: \t", MODULE_TAG); } printf(format __VA_OPT__(,) __VA_ARGS__); printf("\n");
#define LOG_BEDUG(format, ...)                 printf(format __VA_OPT__(,) __VA_ARGS__);

#else /* DEBUG */

#define LOG_TAG_BEDUG(MODULE_TAG, format, ...) {}
#define LOG_BEDUG(format, ...) {}

#endif /* DEBUG */


#define PRINT_MESSAGE(MODULE_TAG, format, ...) printf("%s: \t", MODULE_TAG); printf(format __VA_OPT__(,) __VA_ARGS__);


#ifdef __cplusplus
}
#endif


#endif
