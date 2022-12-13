//
//   SDL_Arduino_INA3221 Library
//   SDL_Arduino_INA3221.cpp Arduino code - runs in continuous mode
//   Version 1.2
//   SwitchDoc Labs   September 2019
//
//

/**************************************************************************/
/*!
    Initial code from INA219 code (Basically just a core structure left)
    @author   K. Townsend (Adafruit Industries)
	@license  BSD (see BSDlicense.txt)

	*/
/**************************************************************************/


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _STM32FXX_HAL_INA3221
#define _STM32FXX_HAL_INA3221

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <stdbool.h>

/*=========================================================================
    I2C ADDRESS/BITS
    -----------------------------------------------------------------------*/
    #define INA3221_ADDRESS                         (0x40)    // 1000000 (A0+A1=GND)
    #define INA3221_READ                            (0x01)
/*=========================================================================*/

/*=========================================================================
    CONFIG REGISTER (R/W)
    -----------------------------------------------------------------------*/
    #define INA3221_REG_CONFIG                      (0x00)
    /*---------------------------------------------------------------------*/
    #define INA3221_CONFIG_RESET                    (0x8000)  // Reset Bit

    #define INA3221_CONFIG_ENABLE_CHAN1             (0x4000)  // Enable Channel 1
    #define INA3221_CONFIG_ENABLE_CHAN2             (0x2000)  // Enable Channel 2
    #define INA3221_CONFIG_ENABLE_CHAN3             (0x1000)  // Enable Channel 3

    #define INA3221_CONFIG_AVG2                     (0x0800)  // AVG Samples Bit 2 - See table 3 spec
    #define INA3221_CONFIG_AVG1                     (0x0400)  // AVG Samples Bit 1 - See table 3 spec
    #define INA3221_CONFIG_AVG0                     (0x0200)  // AVG Samples Bit 0 - See table 3 spec

    #define INA3221_CONFIG_VBUS_CT2                 (0x0100)  // VBUS bit 2 Conversion time - See table 4 spec
    #define INA3221_CONFIG_VBUS_CT1                 (0x0080)  // VBUS bit 1 Conversion time - See table 4 spec
    #define INA3221_CONFIG_VBUS_CT0                 (0x0040)  // VBUS bit 0 Conversion time - See table 4 spec

    #define INA3221_CONFIG_VSH_CT2                  (0x0020)  // Vshunt bit 2 Conversion time - See table 5 spec
    #define INA3221_CONFIG_VSH_CT1                  (0x0010)  // Vshunt bit 1 Conversion time - See table 5 spec
    #define INA3221_CONFIG_VSH_CT0                  (0x0008)  // Vshunt bit 0 Conversion time - See table 5 spec

    #define INA3221_CONFIG_MODE_2                   (0x0004)  // Operating Mode bit 2 - See table 6 spec
    #define INA3221_CONFIG_MODE_1                   (0x0002)  // Operating Mode bit 1 - See table 6 spec
    #define INA3221_CONFIG_MODE_0                   (0x0001)  // Operating Mode bit 0 - See table 6 spec

/*=========================================================================*/

/*=========================================================================
    SHUNT VOLTAGE REGISTER (R)
    -----------------------------------------------------------------------*/
    #define INA3221_REG_SHUNTVOLTAGE_1                (0x01)
/*=========================================================================*/

/*=========================================================================
    BUS VOLTAGE REGISTER (R)
    -----------------------------------------------------------------------*/
    #define INA3221_REG_BUSVOLTAGE_1                  (0x02)
/*=========================================================================*/

#define SHUNT_RESISTOR_VALUE  (8.2)   // default shunt resistor value of 0.1 Ohm

#define INA3221_MAX_TIMEOUT   (0xFFFF)

void INA3221_begin();
bool INA3221_available();
float INA3221_getBusVoltage_V(int channel);
float INA3221_getShuntVoltage_mV(int channel);
float INA3221_getCurrent_mA(int channel);
int INA3221_getManufID();

void INA3221_wireWriteRegister(uint8_t reg, uint16_t value);
void INA3221_wireReadRegister(uint8_t reg, uint16_t *value);
int16_t INA3221_getBusVoltage_raw(int channel);
int16_t INA3221_getShuntVoltage_raw(int channel);

#endif
