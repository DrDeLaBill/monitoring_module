//
//   SDL_Arduino_INA3221 Library
//   SDL_Arduino_INA3221.cpp Arduino code - runs in continuous mode
//   Version 1.2
//   SwitchDoc Labs   September 2019
//
//



/*
    Initial code from INA219 code (Basically just a core structure left)
    @author   K.Townsend (Adafruit Industries)
	@license  BSD (see BSDlicense.txt)
*/

#include "ina3221_sensor.h"

#include "settings_manager.h"

/**************************************************************************/
/*!
    @brief  Checks address availability
*/
/**************************************************************************/
bool INA3221_available()
{
	return HAL_I2C_IsDeviceReady(&INA3221_I2C, (INA3221_ADDRESS << 1), 1, INA3221_MAX_TIMEOUT) == HAL_OK;
}

/**************************************************************************/
/*!
    @brief  Sends a single command byte over I2C
*/
/**************************************************************************/
void INA3221_wireWriteRegister (uint8_t reg, uint16_t value)
{
	uint8_t tx_buffer[3] = {reg, value >> 8, value & 0xFF};
	HAL_I2C_Master_Transmit(&INA3221_I2C, (INA3221_ADDRESS << 1) + 1, tx_buffer, 3, INA3221_MAX_TIMEOUT);
}

/**************************************************************************/
/*!
    @brief  Reads a 16 bit values over I2C
*/
/**************************************************************************/
void INA3221_wireReadRegister(uint8_t reg, uint16_t *value)
{
	uint16_t response = 0;
	HAL_I2C_Master_Transmit(&INA3221_I2C, (INA3221_ADDRESS << 1), &reg, 1, INA3221_MAX_TIMEOUT);
	HAL_I2C_Master_Receive(&INA3221_I2C, (INA3221_ADDRESS << 1), &response, 2, INA3221_MAX_TIMEOUT);
	*value = (response << 8) + (response >> 8);
}

/**************************************************************************/
/*!
    @brief  Setups the HW (defaults to 32V and 2A for calibration values)
*/
/**************************************************************************/
void INA3221_begin()
{
	// Set chip to known config values to start
	// Set Config register to take into account the settings above
	uint16_t config = INA3221_CONFIG_ENABLE_CHAN1 |
					INA3221_CONFIG_ENABLE_CHAN2 |
					INA3221_CONFIG_ENABLE_CHAN3 |
					INA3221_CONFIG_AVG1 |
					INA3221_CONFIG_VBUS_CT2 |
					INA3221_CONFIG_VSH_CT2 |
					INA3221_CONFIG_MODE_2 |
					INA3221_CONFIG_MODE_1 |
					INA3221_CONFIG_MODE_0;
	INA3221_wireWriteRegister(INA3221_REG_CONFIG, config);
}

/**************************************************************************/
/*!
    @brief  Gets the raw bus voltage (16-bit signed integer, so +-32767)
*/
/**************************************************************************/
int16_t INA3221_getBusVoltage_raw(int channel)
{
	uint16_t value;
	INA3221_wireReadRegister(INA3221_REG_BUSVOLTAGE_1+(channel -1) *2, &value);
	// Shift to the right 3 to drop CNVR and OVF and multiply by LSB
	return (int16_t)(value);
}

/**************************************************************************/
/*!
    @brief  Gets the raw shunt voltage (16-bit signed integer, so +-32767)
*/
/**************************************************************************/
int16_t INA3221_getShuntVoltage_raw(int channel)
{
	uint16_t value;
	INA3221_wireReadRegister(INA3221_REG_SHUNTVOLTAGE_1+(channel -1) *2, &value);
	return (int16_t)value;
}



/**************************************************************************/
/*!
    @brief  Gets the shunt voltage in mV (so +-168.3mV)
*/
/**************************************************************************/
float INA3221_getShuntVoltage_mV(int channel)
{
	int16_t value;
	value = INA3221_getShuntVoltage_raw(channel);
	return value * 0.005;
}

/**************************************************************************/
/*!
    @brief  Gets the shunt voltage in volts
*/
/**************************************************************************/
float INA3221_getBusVoltage_V(int channel)
{
	int16_t value = INA3221_getBusVoltage_raw(channel);
	return value * 0.001;
}

/**************************************************************************/
/*!
    @brief  Gets the current value in mA, taking into account the
            config settings and current LSB
*/
/**************************************************************************/
float INA3221_getCurrent_mA(int channel)
{
	float valueDec = INA3221_getShuntVoltage_mV(channel) / SHUNT_RESISTOR_VALUE;
	return valueDec;
}


/**************************************************************************/
/*!
    @brief  Gets the Manufacturers ID
*/
/**************************************************************************/
int INA3221_getManufID()
{
	uint16_t value;
	INA3221_wireReadRegister(0xFE, &value);
	return value;
}
