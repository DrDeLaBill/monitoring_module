/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>

#include "glog.h"
#include "pump.h"
#include "soul.h"
#include "gutils.h"
#include "system.h"
#include "settings.h"
#include "sim_module.h"
#include "ds1307_driver.h"
#include "liquid_sensor.h"
#include "command_manager.h"
#include "pressure_sensor.h"

#include "RecordDB.h"
#include "StorageAT.h"
#include "SoulGuard.h"
#include "LogService.h"
#include "StorageDriver.h"

#ifdef EEPROM_MODE
#   include "at24cm01.h"
#else
#   include "w25qxx.h"
#endif
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

const char MAIN_TAG[] = "MAIN";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void error_loop();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

StorageDriver storageDriver;
#ifdef EEPROM_MODE
StorageAT storage(
	eeprom_get_size() / STORAGE_PAGE_SIZE,
	&storageDriver,
    EEPROM_PAGE_SIZE
);
#else
StorageAT* storage;
#endif

char cmd_input_chr = 0;
char sim_input_chr = 0;

SoulGuard<
	RestartWatchdog,
	PowerWatchdog,
	StackWatchdog
> hardGuard;
SoulGuard<
	MemoryWatchdog,
	SettingsWatchdog,
	RTCWatchdog
> softGuard;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	system_pre_load();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  if (is_error(RCC_ERROR)) {
	  system_clock_hsi_config();
  } else {
	  set_error(RCC_ERROR);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  	  reset_error(RCC_ERROR);
  }
#ifdef EEPROM_MODE
#   ifndef DEBUG
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_IWDG_Init();
  MX_RTC_Init();
#   else
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
#   endif
#else
#   ifndef DEBUG
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_IWDG_Init();
  MX_RTC_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
#   else
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  MX_SPI2_Init();
#   endif
#endif

    HAL_Delay(100);

	SystemInfo();

    set_status(LOADING);

    // UART command manager
    command_manager_begin();
    // Pump
    pump_init();
    // Clock
    DS1307_Init();
    // Commands
    HAL_UART_Receive_IT(&COMMAND_UART, (uint8_t*) &cmd_input_chr, sizeof(char));
    // Sim module
    HAL_UART_Receive_IT(&SIM_MODULE_UART, (uint8_t*) &sim_input_chr, sizeof(char));

    gprint("\n\n\n");
    printTagLog(MAIN_TAG, "The device is loading");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    utl::Timer errTimer(40 * SECOND_MS);

    set_error(STACK_ERROR);
	while (has_errors()) {
		hardGuard.defend();

    	if (!errTimer.wait()) {
			system_error_handler((SOUL_STATUS)get_first_error(), error_loop);
		}
    }

#ifndef EEPROM_MODE
    set_error(MEMORY_INIT_ERROR);
    errTimer.start();
    while(flash_w25qxx_init() != FLASH_OK) {
    	if (!errTimer.wait()) {
			system_error_handler(MEMORY_INIT_ERROR, error_loop);
		}
    }
    reset_error(MEMORY_INIT_ERROR);

    storage = new StorageAT(
		flash_w25qxx_get_pages_count(),
		&storageDriver,
		FLASH_W25_SECTOR_SIZE
	);
#endif

    errTimer.start();
	while (has_errors() || is_status(LOADING)) {
		hardGuard.defend();
		softGuard.defend();

    	if (!errTimer.wait()) {
			system_error_handler((SOUL_STATUS)get_first_error(), error_loop);
		}
    }

	system_rtc_test();

    sim_begin();

    system_post_load();

    HAL_Delay(100);

    printTagLog(MAIN_TAG, "The device has been loaded\n");

#ifdef DEBUG
	unsigned last_error = get_first_error();

	unsigned kFLOPScounter = 0;
	utl::Timer kFLOPSTimer(10 * SECOND_MS);
	kFLOPSTimer.start();
#endif

	set_status(WORKING);
	set_status(HAS_NEW_RECORD);
	errTimer.start();
    while (1) {
    	hardGuard.defend();
    	softGuard.defend();

#ifdef DEBUG
		unsigned error = get_first_error();
		if (error && last_error != error) {
			printTagLog(MAIN_TAG, "New error: %u", error);
			last_error = error;
		} else if (last_error != error) {
			printTagLog(MAIN_TAG, "No errors");
			last_error = error;
		}

		kFLOPScounter++;
		if (!kFLOPSTimer.wait()) {
			printTagLog(
				MAIN_TAG,
				"kFLOPS: %u.%u",
				kFLOPScounter / (10 * SECOND_MS),
				(kFLOPScounter / SECOND_MS) % 10
			);
			kFLOPScounter = 0;
			kFLOPSTimer.start();
		}
#endif

		if (!errTimer.wait()) {
			system_error_handler((SOUL_STATUS)get_first_error(), error_loop);
		}

        // Pump
        pump_proccess();

		if (has_errors() || is_status(LOADING)) {
			continue;
		}

#ifndef DEBUG
		HAL_IWDG_Refresh(&DEVICE_IWDG);
#endif
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        // Commands from UART
        command_manager_proccess();

        // Shunt sensor
        pressure_sensor_proccess();

        // Sim module
        sim_proccess();

        // Liquid level proccess
        level_tick();

        // Logger
        LogService::update();

		errTimer.start();
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_ADC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void error_loop()
{
	hardGuard.defend();
	softGuard.defend();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &COMMAND_UART) {
        cmd_proccess_input(cmd_input_chr);
        HAL_UART_Receive_IT(&COMMAND_UART, (uint8_t*) &cmd_input_chr, 1);
    }
    if (huart == &SIM_MODULE_UART) {
        sim_proccess_input(sim_input_chr);
        HAL_UART_Receive_IT(&SIM_MODULE_UART, (uint8_t*) &sim_input_chr, 1);
    }
}

int _write(int, uint8_t *ptr, int len) {
    HAL_UART_Transmit(&BEDUG_UART, (uint8_t*)ptr, static_cast<uint16_t>(len), GENERAL_TIMEOUT_MS);
#ifdef DEBUG
    for (int DataIdx = 0; DataIdx < len; DataIdx++) {
        ITM_SendChar(*ptr++);
    }
    return len;
#endif
    return 0;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
#ifdef DEBUG
    b_assert(__FILE__, __LINE__, "The error handler has been called");
#endif
    SOUL_STATUS err = has_errors() ? (SOUL_STATUS)get_first_error() : ERROR_HANDLER_CALLED;
	system_error_handler(err, error_loop);
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
#ifdef DEBUG
	b_assert((char*)file, line, "Wrong parameters value");
#endif
	SOUL_STATUS err = has_errors() ? (SOUL_STATUS)get_first_error() : ASSERT_ERROR;
	system_error_handler(err, error_loop);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
