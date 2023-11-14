#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(STM32F100xB) || \
    defined(STM32F100xE) || \
    defined(STM32F101x6) || \
    defined(STM32F101xB) || \
    defined(STM32F101xE) || \
    defined(STM32F101xG) || \
    defined(STM32F102x6) || \
    defined(STM32F102xB) || \
    defined(STM32F103x6) || \
    defined(STM32F103xB) || \
    defined(STM32F103xE) || \
    defined(STM32F103xG) || \
    defined(STM32F105xC) || \
    defined(STM32F107xC)
    #include "stm32f1xx_hal.h"
#elif defined(STM32F405xx) || \
    defined(STM32F415xx) || \
    defined(STM32F407xx) || \
    defined(STM32F417xx) || \
    defined(STM32F427xx) || \
    defined(STM32F437xx) || \
    defined(STM32F429xx) || \
    defined(STM32F439xx) || \
    defined(STM32F401xC) || \
    defined(STM32F401xE) || \
    defined(STM32F410Tx) || \
    defined(STM32F410Cx) || \
    defined(STM32F410Rx) || \
    defined(STM32F411xE) || \
    defined(STM32F446xx) || \
    defined(STM32F469xx) || \
    defined(STM32F479xx) || \
    defined(STM32F412Cx) || \
    defined(STM32F412Zx) || \
    defined(STM32F412Rx) || \
    defined(STM32F412Vx) || \
    defined(STM32F413xx) || \
    defined(STM32F423xx)
    #include "stm32f4xx_hal.h"
#else
    #error "Please select first the target STM32Fxxx device used in your application (in eeprom_at24cm01_storage.c file)"
#endif



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

#ifndef STRUCT_MEMBER_NOWARN_UNSAFE
#define STRUCT_MEMBER_NOWARN_UNSAFE(struct_type, struct_ptr, member_name) \
  ((void*)((((char*)(struct_ptr)) + offsetof(struct_type, member_name))))
#endif


typedef struct _util_timer_t {
    uint32_t start;
    uint32_t delay;
} util_timer_t;


void     util_timer_start(util_timer_t* tm, uint32_t waitMs);
bool     util_is_timer_wait(util_timer_t* tm);


typedef struct _util_port_pin_t {
    GPIO_TypeDef* port;
    uint16_t      pin;
} util_port_pin_t;


void     util_debug_hex_dump(const char* tag, const uint8_t* buf, uint32_t start_counter, uint16_t len);
int      util_convert_range(int val, int rngl1, int rngh1, int rngl2, int rngh2);
uint16_t util_get_crc16(uint8_t* buf, uint16_t len);
bool     util_wait_event(bool (*condition) (void), uint32_t time);
uint8_t  util_get_number_len(int number);


#ifdef DEBUG

#define LOG_TAG_BEDUG(MODULE_TAG, format, ...) if (strlen(MODULE_TAG)) { printf("%09lu->%s:\t", HAL_GetTick(), MODULE_TAG); } printf(format __VA_OPT__(,) __VA_ARGS__); printf("\n");
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
