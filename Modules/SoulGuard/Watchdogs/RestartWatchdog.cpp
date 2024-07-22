/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "glog.h"
#include "main.h"
#include "hal_defs.h"

#include "CodeStopwatch.h"


bool RestartWatchdog::flagsCleared = false;


void RestartWatchdog::check()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

	if (flagsCleared) {
		return;
	}

	bool flag = false;
	// IWDG check reboot
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
		printTagLog(TAG, "IWDG just went off");
		flag = true;
	}

	// WWDG check reboot
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
		printTagLog(TAG, "WWDG just went off");
		flag = true;
	}

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
		printTagLog(TAG, "SOFT RESET");
		flag = true;
	}

	if (flag) {
		__HAL_RCC_CLEAR_RESET_FLAGS();
		printTagLog(TAG, "DEVICE HAS BEEN REBOOTED");
		RestartWatchdog::reset_i2c_errata(); // TODO: move reset_i2c to memory watchdog
		HAL_Delay(2500);
	}
	// TODO: IWDG, NVIC_SysReset and other restarts detect and reset
//	BEDUG_ASSERT(false, "INTERNAL ERROR HAS BEEN OCCURRED (HARD FAULT)");
}

void RestartWatchdog::reset_i2c_errata()
{
	printTagLog(TAG, "RESET I2C (ERRATA)");

//	EEPROM_I2C.Instance->CR1 |= I2C_CR1_SWRST;
//	HAL_Delay(GENERAL_TIMEOUT_MS);
//	EEPROM_I2C.Instance->CR1 &= ~I2C_CR1_SWRST;

	HAL_I2C_DeInit(&EEPROM_I2C);

	GPIO_TypeDef* I2C_PORT = GPIOB;
	uint16_t I2C_SDA_Pin = GPIO_PIN_7;
	uint16_t I2C_SCL_Pin = GPIO_PIN_6;

	GPIO_InitTypeDef GPIO_InitStruct = { };
	GPIO_InitStruct.Pin   = I2C_SCL_Pin | I2C_SCL_Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);

	hi2c1.Instance->CR1 &= ~(0x0001);

	GPIO_InitTypeDef GPIO_InitStructure = {};
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;


	GPIO_InitStructure.Pin = I2C_SCL_Pin;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
	HAL_GPIO_WritePin(I2C_PORT, static_cast<uint16_t>(GPIO_InitStructure.Pin), GPIO_PIN_SET);

	GPIO_InitStructure.Pin = I2C_SDA_Pin;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
	HAL_GPIO_WritePin(I2C_PORT, static_cast<uint16_t>(GPIO_InitStructure.Pin), GPIO_PIN_SET);

	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL_Pin)) asm("nop");
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_Pin)) asm("nop");

	HAL_GPIO_WritePin(I2C_PORT, I2C_SDA_Pin, GPIO_PIN_RESET);
	while(GPIO_PIN_RESET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_Pin)) asm("nop");

	HAL_GPIO_WritePin(I2C_PORT, I2C_SCL_Pin, GPIO_PIN_RESET);
	while(GPIO_PIN_RESET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL_Pin)) asm("nop");

	HAL_GPIO_WritePin(I2C_PORT, I2C_SDA_Pin, GPIO_PIN_SET);
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_Pin)) asm("nop");

	HAL_GPIO_WritePin(I2C_PORT, I2C_SCL_Pin, GPIO_PIN_SET);
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL_Pin)) asm("nop");

	GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;

	GPIO_InitStructure.Pin = I2C_SCL_Pin;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = I2C_SDA_Pin;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStructure);

	EEPROM_I2C.Instance->CR1 |= 0x8000;
	asm("nop");
	EEPROM_I2C.Instance->CR1 &= ~0x8000;
	asm("nop");

	EEPROM_I2C.Instance->CR1 |= 0x0001;

	HAL_I2C_Init(&EEPROM_I2C);
}
