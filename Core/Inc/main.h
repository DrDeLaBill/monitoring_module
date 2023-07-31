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

int _write(int file, uint8_t *ptr, int len);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LIQUID_LEVEL_Pin GPIO_PIN_0
#define LIQUID_LEVEL_GPIO_Port GPIOA
#define LIQUID_PRESS1_Pin GPIO_PIN_1
#define LIQUID_PRESS1_GPIO_Port GPIOA
#define RS485_TX_Pin GPIO_PIN_2
#define RS485_TX_GPIO_Port GPIOA
#define RS485_RX_Pin GPIO_PIN_3
#define RS485_RX_GPIO_Port GPIOA
#define PUMP_Pin GPIO_PIN_12
#define PUMP_GPIO_Port GPIOB
#define SIM_RESET_Pin GPIO_PIN_8
#define SIM_RESET_GPIO_Port GPIOA
#define RED_LED_Pin GPIO_PIN_15
#define RED_LED_GPIO_Port GPIOA
#define GREEN_LED_Pin GPIO_PIN_3
#define GREEN_LED_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
// UART command manager
extern UART_HandleTypeDef     huart3;
#define COMMAND_UART          huart3
// Liquid sensor
extern ADC_HandleTypeDef      hadc1;
#define MEASURE_ADC           hadc1
// SIM module
extern UART_HandleTypeDef     huart1;
#define SIM_MODULE_UART       huart1
#define SIM_MODULE_RESET_PORT SIM_RESET_GPIO_Port
#define SIM_MODULE_RESET_PIN  SIM_RESET_Pin

// EEPROM
extern I2C_HandleTypeDef      hi2c1;
#define EEPROM_I2C            hi2c1
// Clock
extern RTC_HandleTypeDef      hrtc;
#define CLOCK_RTC             hrtc
// Watchdog
extern IWDG_HandleTypeDef     hiwdg;
#define DEVICE_IWDG           hiwdg
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
