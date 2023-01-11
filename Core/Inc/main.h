/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PUMP_LED_Pin GPIO_PIN_15
#define PUMP_LED_GPIO_Port GPIOC
#define PUMP_Pin GPIO_PIN_0
#define PUMP_GPIO_Port GPIOA
#define LIQUID_Pin GPIO_PIN_1
#define LIQUID_GPIO_Port GPIOA
#define SPI1_SD_NSS_Pin GPIO_PIN_4
#define SPI1_SD_NSS_GPIO_Port GPIOA
#define SIM_RESET_Pin GPIO_PIN_12
#define SIM_RESET_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

//Settings
#define SETTINGS_UART huart2;
//SD card
extern SPI_HandleTypeDef hspi2;
#define SD_HSPI hspi2
#define SD_CS_GPIO_Port SPI1_SD_NSS_GPIO_Port
#define SD_CS_Pin SPI1_SD_NSS_Pin
// Shunt
extern I2C_HandleTypeDef hi2c1;
#define INA3221_I2C hi2c1
// UART command manager
extern UART_HandleTypeDef huart2;
#define COMMAND_UART huart2
// Liquid sensor
extern ADC_HandleTypeDef hadc1;
#define LIQUID_ADC hadc1
// SIM module
extern UART_HandleTypeDef huart1;
#define SIM_MODULE_UART huart1
#define SIM_MODULE_RESET_PORT SIM_RESET_GPIO_Port
#define SIM_MODULE_RESET_PIN SIM_RESET_Pin
// Clock
extern I2C_HandleTypeDef hi2c1;
#define CLOCK_I2C hi2c1

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
