#ifndef INC_DS1307_DRIVER_H_
#define INC_DS1307_DRIVER_H_


#ifdef __cplusplus
extern "C"{
#endif


#include <stdint.h>

#include "stm32f1xx_hal.h"


typedef enum _DS1307_STATUS {
	DS1307_OK,
	DS1307_ERROR
} DS1307_STATUS;


/*----------------------------------------------------------------------------*/
#define DS1307_I2C_ADDR 	    0x68
#define DS1307_REG_SECOND 	    0x00
#define DS1307_REG_MINUTE 	    0x01
#define DS1307_REG_HOUR  	    0x02
#define DS1307_REG_DOW    	    0x03
#define DS1307_REG_DATE   	    0x04
#define DS1307_REG_MONTH  	    0x05
#define DS1307_REG_YEAR   	    0x06
#define DS1307_REG_CONTROL 	    0x07
#define DS1307_REG_RAM_UTC_HR   0x08
#define DS1307_REG_RAM_UTC_MIN	0x09
#define DS1307_REG_RAM_CENT    	0x0A
#define DS1307_REG_RAM_RDY_BE	0x0B
#define DS1307_REG_RAM_RDY_DA	0x0C
#define DS1307_REG_RAM_RDY_CO   0x0D
#define DS1307_REG_RAM_RDY_DE	0x0E
#define DS1307_REG_RAM_RDY   	0x0F
#define DS1307_REG_RAM   	    0x10
#define DS1307_REG_RAM_END      0x3F
#define DS1307_TIMEOUT		    1000
/*----------------------------------------------------------------------------*/
extern I2C_HandleTypeDef *_ds1307_ui2c;

typedef enum DS1307_Rate{
	DS1307_1HZ, DS1307_4096HZ, DS1307_8192HZ, DS1307_32768HZ
} DS1307_Rate;

typedef enum DS1307_SquareWaveEnable{
	DS1307_DISABLED, DS1307_ENABLED
} DS1307_SquareWaveEnable;

DS1307_STATUS DS1307_Init();

DS1307_STATUS DS1307_SetClockHalt(uint8_t halt);
DS1307_STATUS DS1307_GetClockHalt(uint8_t* res);


DS1307_STATUS DS1307_SetRegByte(uint8_t regAddr, uint8_t val);
DS1307_STATUS DS1307_GetRegByte(uint8_t regAddr, uint8_t* res);

DS1307_STATUS DS1307_SetEnableSquareWave(DS1307_SquareWaveEnable mode);
DS1307_STATUS DS1307_SetInterruptRate(DS1307_Rate rate);

DS1307_STATUS DS1307_GetDayOfWeek(uint8_t* res);
DS1307_STATUS DS1307_GetDate(uint8_t* res);
DS1307_STATUS DS1307_GetMonth(uint8_t* res);
DS1307_STATUS DS1307_GetYear(uint16_t* res);

DS1307_STATUS DS1307_GetHour(uint8_t* res);
DS1307_STATUS DS1307_GetMinute(uint8_t* res);
DS1307_STATUS DS1307_GetSecond(uint8_t* res);
DS1307_STATUS DS1307_GetTimeZoneHour(int8_t* res);
DS1307_STATUS DS1307_GetTimeZoneMin(int8_t* res);

DS1307_STATUS DS1307_SetDayOfWeek(uint8_t dow);
DS1307_STATUS DS1307_SetDate(uint8_t date);
DS1307_STATUS DS1307_SetMonth(uint8_t month);
DS1307_STATUS DS1307_SetYear(uint16_t year);

DS1307_STATUS DS1307_SetHour(uint8_t hour_24mode);
DS1307_STATUS DS1307_SetMinute(uint8_t minute);
DS1307_STATUS DS1307_SetSecond(uint8_t second);
DS1307_STATUS DS1307_SetTimeZone(int8_t hr, uint8_t min);

DS1307_STATUS DS1307_SetInitialized(uint8_t);
DS1307_STATUS DS1307_GetInitialized(uint8_t*);

uint8_t DS1307_DecodeBCD(uint8_t bin);
uint8_t DS1307_EncodeBCD(uint8_t dec);


#ifdef __cplusplus
}
#endif

#endif /* INC_DS1307_DRIVER_H_ */
