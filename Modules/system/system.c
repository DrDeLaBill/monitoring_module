/* Copyright © 2023 Georgy E. All rights reserved. */

#include "system.h"

#include <stdbool.h>

#ifndef NO_SYSTEM_RTC_TEST
#   include "clock.h"
#endif
#include "main.h"
#include "glog.h"
#include "hal_defs.h"


const char SYSTEM_TAG[] = "SYS";


uint16_t SYSTEM_ADC_VOLTAGE[3] = {0};
bool system_hsi_initialized = false;


#ifndef IS_SAME_TIME
#   define IS_SAME_TIME(TIME1, TIME2) (TIME1.Hours   == TIME2.Hours && \
                                       TIME1.Minutes == TIME2.Minutes && \
									   TIME1.Seconds == TIME2.Seconds)
#endif


void system_clock_hsi_config(void)
{
#ifdef STM32F1
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		return;
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
							  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		return;
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_ADC;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		return;
	}
#elif defined(STM32F4)
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	*/
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
										|RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 84;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		return;
	}

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
									|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		return;
	}
#else
#   error "Please select your controller"
#endif

	system_hsi_initialized = true;
}

void system_rtc_test(void)
{
#ifndef NO_SYSTEM_RTC_TEST
#   ifdef DEBUG
	static const char TEST_TAG[] = "TEST";
	gprint("\n\n\n");
	printTagLog(TEST_TAG, "RTC testing in progress...");
#   endif

	RTC_DateTypeDef readDate  ={0};
	RTC_TimeTypeDef readTime = {0};

#   if SYSTEM_BEDUG
	printPretty("Get date test: ");
#   endif
	if (!clock_get_rtc_date(&readDate)) {
#   if SYSTEM_BEDUG
		gprint("   error\n");
#   endif
		system_error_handler(RTC_ERROR, NULL);
	}
#   if SYSTEM_BEDUG
	gprint("   OK\n");
	printPretty("Get time test: ");
#   endif
	if (!clock_get_rtc_time(&readTime)) {
#   if SYSTEM_BEDUG
		gprint("   error\n");
#   endif
		system_error_handler(RTC_ERROR, NULL);
	}
#   if SYSTEM_BEDUG
	gprint("   OK\n");
	printPretty("Save date test: ");
#   endif
	if (!clock_save_date(&readDate)) {
#   if SYSTEM_BEDUG
		gprint("  error\n");
#   endif
		system_error_handler(RTC_ERROR, NULL);
	}
#   if SYSTEM_BEDUG
	gprint("  OK\n");
	printPretty("Save time test: ");
#   endif
	if (!clock_save_time(&readTime)) {
#   if SYSTEM_BEDUG
		gprint("  error\n");
#   endif
		system_error_handler(RTC_ERROR, NULL);
	}
#   if SYSTEM_BEDUG
	gprint("  OK\n");
#   endif


	RTC_DateTypeDef checkDate  ={0};
	RTC_TimeTypeDef checkTime = {0};
#   if SYSTEM_BEDUG
	printPretty("Check date test: ");
#   endif
	if (!clock_get_rtc_date(&checkDate)) {
#   if SYSTEM_BEDUG
		gprint(" error\n");
#   endif
		system_error_handler(RTC_ERROR, NULL);
	}
	if (memcmp((void*)&readDate, (void*)&checkDate, sizeof(readDate))) {
#   if SYSTEM_BEDUG
		gprint(" error\n");
#   endif
		system_error_handler(RTC_ERROR, NULL);
	}
#   if SYSTEM_BEDUG
	gprint(" OK\n");
	printPretty("Check time test: ");
#   endif
	if (!clock_get_rtc_time(&checkTime)) {
#   if SYSTEM_BEDUG
		gprint(" error\n");
#   endif
		system_error_handler(RTC_ERROR, NULL);
	}
	if (!IS_SAME_TIME(readTime, checkTime)) {
#   if SYSTEM_BEDUG
		gprint(" error\n");
#   endif
		system_error_handler(RTC_ERROR, NULL);
	}
#   if SYSTEM_BEDUG
	gprint(" OK\n");
#   endif


#   if SYSTEM_BEDUG
	printPretty("Weekday test\n");
#   endif
	const RTC_DateTypeDef dates[] = {
		{RTC_WEEKDAY_SATURDAY,  01, 01, 00},
		{RTC_WEEKDAY_SUNDAY,    01, 02, 00},
		{RTC_WEEKDAY_SATURDAY,  04, 27, 24},
		{RTC_WEEKDAY_SUNDAY,    04, 28, 24},
		{RTC_WEEKDAY_MONDAY,    04, 29, 24},
		{RTC_WEEKDAY_TUESDAY,   04, 30, 24},
		{RTC_WEEKDAY_WEDNESDAY, 05, 01, 24},
		{RTC_WEEKDAY_THURSDAY,  05, 02, 24},
		{RTC_WEEKDAY_FRIDAY,    05, 03, 24},
	};
#   if defined(STM32F1)
	const RTC_TimeTypeDef times[] = {
		{00, 00, 00},
		{00, 00, 00},
		{03, 24, 49},
		{04, 14, 24},
		{03, 27, 01},
		{23, 01, 40},
		{03, 01, 40},
		{04, 26, 12},
		{03, 52, 35},
	};
#   elif defined(STM32F4)
	const RTC_TimeTypeDef times[] = {
		{00, 00, 00, 0, 0, 0, 0, 0},
		{00, 00, 00, 0, 0, 0, 0, 0},
		{03, 24, 49, 0, 0, 0, 0, 0},
		{04, 14, 24, 0, 0, 0, 0, 0},
		{03, 27, 01, 0, 0, 0, 0, 0},
		{23, 01, 40, 0, 0, 0, 0, 0},
		{03, 01, 40, 0, 0, 0, 0, 0},
		{04, 26, 12, 0, 0, 0, 0, 0},
		{03, 52, 35, 0, 0, 0, 0, 0},
	};
#   endif
	const uint32_t seconds[] = {
		0,
		86400,
		767503489,
		767592864,
		767676421,
		767833300,
		767847700,
		767939172,
		768023555,
	};

	for (unsigned i = 0; i < __arr_len(seconds); i++) {
#   if SYSTEM_BEDUG
		printPretty("[%02u]: ", i);
#   endif

		RTC_DateTypeDef tmpDate = {0};
		RTC_TimeTypeDef tmpTime = {0};
		clock_seconds_to_datetime(seconds[i], &tmpDate, &tmpTime);
		if (memcmp((void*)&tmpDate, (void*)&dates[i], sizeof(tmpDate))) {
#   if SYSTEM_BEDUG
			gprint("            error\n");
#   endif
			system_error_handler(RTC_ERROR, NULL);
		}
		if (!IS_SAME_TIME(tmpTime, times[i])) {
#   if SYSTEM_BEDUG
			gprint("            error\n");
#   endif
			system_error_handler(RTC_ERROR, NULL);
		}

		uint32_t tmpSeconds = clock_datetime_to_seconds(&dates[i], &times[i]);
		if (tmpSeconds != seconds[i]) {
#   if SYSTEM_BEDUG
			gprint("            error\n");
#   endif
			system_error_handler(RTC_ERROR, NULL);
		}

#   if SYSTEM_BEDUG
		gprint("            OK\n");
#   endif
	}


#   if SYSTEM_BEDUG
	printTagLog(TEST_TAG, "RTC testing done");
#   endif

#endif
}

void system_pre_load(void)
{
	if (!MCUcheck()) {
		set_error(MCU_ERROR);
		system_error_handler(get_first_error(), NULL);
		while(1) {}
	}

	__set_bit(RCC->CR, RCC_CR_HSEON_Pos);

	unsigned counter = 0;
	while (1) {
		if (__get_bit(RCC->CR, RCC_CR_HSERDY_Pos)) {
			__reset_bit(RCC->CR, RCC_CR_HSEON_Pos);
			break;
		}

		if (counter > 0x100) {
			set_status(RCC_FAULT);
			set_error(RCC_ERROR);
			break;
		}

		counter++;
	}

#ifdef STM32F1
	uint32_t backupregister = (uint32_t)BKP_BASE;
	backupregister += (RTC_BKP_DR1 * 4U);
	SOUL_STATUS status = (SOUL_STATUS)((*(__IO uint32_t *)(backupregister)) & BKP_DR1_D);
#elif defined(STM32F4)
	HAL_PWR_EnableBkUpAccess();
	SOUL_STATUS status = (SOUL_STATUS)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
	HAL_PWR_DisableBkUpAccess();
#endif

	set_last_error(status);

    if (status == MEMORY_ERROR) {
     	set_error(MEMORY_ERROR);
    } else if (status == STACK_ERROR) {
    	set_error(STACK_ERROR);
    } else if (status == SETTINGS_LOAD_ERROR) {
    	set_error(SETTINGS_LOAD_ERROR);
    }
}

void system_post_load(void)
{
	extern RTC_HandleTypeDef hrtc;
	extern ADC_HandleTypeDef hadc1;

	HAL_PWR_EnableBkUpAccess();
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0);
	HAL_PWR_DisableBkUpAccess();

#if SYSTEM_BEDUG
	if (get_last_error()) {
		printTagLog(SYSTEM_TAG, "Last reload error: %u", get_last_error());
	}
#endif


#ifdef STM32F1
	HAL_ADCEx_Calibration_Start(&hadc1);
#endif
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)SYSTEM_ADC_VOLTAGE, 3);
	uint64_t counter = 0;
	uint64_t count_max = HAL_RCC_GetHCLKFreq() * 10;
	util_old_timer_t timer = {0};
	util_old_timer_start(&timer, 10000);
	while (1) {
		uint32_t voltage = get_system_power();
		if (STM_MIN_VOLTAGEx10 <= voltage && voltage <= STM_MAX_VOLTAGEx10) {
			break;
		}

		if (is_error(RCC_ERROR) && counter > count_max) {
			set_error(POWER_ERROR);
			break;
		} else if (!util_old_timer_wait(&timer)) {
			set_error(POWER_ERROR);
			break;
		}

		counter++;
	}

	if (has_errors()) {
		system_error_handler(
			(get_first_error() == INTERNAL_ERROR) ?
				LOAD_ERROR :
				(SOUL_STATUS)get_first_error(),
			NULL
		);
	}
}

void system_error_handler(SOUL_STATUS error, void (*error_loop) (void))
{
	extern RTC_HandleTypeDef hrtc;

	static bool called = false;
	if (called) {
		return;
	}
	called = true;

	set_error(error);

	if (!has_errors()) {
		error = INTERNAL_ERROR;
	}

#if SYSTEM_BEDUG
	printTagLog(SYSTEM_TAG, "system_error_handler called error=%u", error);
#endif

	if (is_error(RCC_ERROR) && !system_hsi_initialized) {
		system_clock_hsi_config();
	}

	bool rtc_initialized = true;
	if (!hrtc.Instance) {
		hrtc.Instance = RTC;
		hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
		hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
		if (HAL_RTC_Init(&hrtc) != HAL_OK) {
			rtc_initialized = false;
		}
	}

	/* Custom events begin */
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitStruct.Pin = RED_LED_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = PUMP_LAMP_Pin|GREEN_LED_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(PUMP_LAMP_GPIO_Port, PUMP_LAMP_Pin, GPIO_PIN_SET);
	util_old_timer_t led_timer = {0};
	/* Custom events end */

	if (rtc_initialized) {
		HAL_PWR_EnableBkUpAccess();
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, error);
		HAL_PWR_DisableBkUpAccess();
	}

	uint64_t counter = 0;
	uint64_t count_max = HAL_RCC_GetHCLKFreq() * 10;
	util_old_timer_t timer = {0};
	util_old_timer_start(&timer, 10000);
	while(1) {
		if (!is_error(RCC_ERROR) && error_loop) {
			error_loop();
		}

		/* Custom events begin */
		if (!util_old_timer_wait(&led_timer)) {
			util_old_timer_start(&led_timer, 300);
			HAL_GPIO_TogglePin(PUMP_LAMP_GPIO_Port, PUMP_LAMP_Pin);
			HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
			HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
		}
		/* Custom events end */


		if (is_error(RCC_ERROR) && counter > count_max) {
			set_error(POWER_ERROR);
			break;
		} else if (!util_old_timer_wait(&timer)) {
			set_error(POWER_ERROR);
			break;
		}

		counter++;
	}

#if SYSTEM_BEDUG
	printTagLog(SYSTEM_TAG, "system reset");
	counter = 100;
	while(counter--);
#endif

	NVIC_SystemReset();
}

uint32_t get_system_power(void)
{
	if (!SYSTEM_ADC_VOLTAGE[0]) {
		return 0;
	}
	return (STM_ADC_MAX * STM_REF_VOLTAGEx10) / SYSTEM_ADC_VOLTAGE[0];
}

void system_reset_i2c_errata(void)
{
#ifndef NO_SYSTEM_I2C_RESET
#if SYSTEM_BEDUG
	printTagLog(SYSTEM_TAG, "RESET I2C (ERRATA)");
#endif

	if (!EEPROM_I2C.Instance) {
		return;
	}

	HAL_I2C_DeInit(&EEPROM_I2C);

	GPIO_TypeDef* I2C_PORT = GPIOB;
	uint16_t I2C_SDA_Pin = GPIO_PIN_7;
	uint16_t I2C_SCL_Pin = GPIO_PIN_6;

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin   = I2C_SCL_Pin | I2C_SCL_Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);

	hi2c1.Instance->CR1 &= (unsigned)~(0x0001);

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
//	GPIO_InitStructure.Alternate = 0;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;


	GPIO_InitStructure.Pin = I2C_SCL_Pin;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
	HAL_GPIO_WritePin(I2C_PORT, (uint16_t)(GPIO_InitStructure.Pin), GPIO_PIN_SET);

	GPIO_InitStructure.Pin = I2C_SDA_Pin;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
	HAL_GPIO_WritePin(I2C_PORT, (uint16_t)(GPIO_InitStructure.Pin), GPIO_PIN_SET);

	util_old_timer_t timer = {0};
	util_old_timer_start(&timer, 10000);
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL_Pin)) {
		if (!util_old_timer_wait(&timer)) {
			system_error_handler(I2C_ERROR, NULL);
		}
		asm("nop");
	}
	util_old_timer_start(&timer, 10000);
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_Pin)) {
		if (!util_old_timer_wait(&timer)) {
			system_error_handler(I2C_ERROR, NULL);
		}
		asm("nop");
	}

	HAL_GPIO_WritePin(I2C_PORT, I2C_SDA_Pin, GPIO_PIN_RESET);
	util_old_timer_start(&timer, 10000);
	while(GPIO_PIN_RESET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_Pin)) {
		if (!util_old_timer_wait(&timer)) {
			system_error_handler(I2C_ERROR, NULL);
		}
		asm("nop");
	}

	HAL_GPIO_WritePin(I2C_PORT, I2C_SCL_Pin, GPIO_PIN_RESET);
	util_old_timer_start(&timer, 10000);
	while(GPIO_PIN_RESET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL_Pin)) {
		if (!util_old_timer_wait(&timer)) {
			system_error_handler(I2C_ERROR, NULL);
		}
		asm("nop");
	}

	HAL_GPIO_WritePin(I2C_PORT, I2C_SDA_Pin, GPIO_PIN_SET);
	util_old_timer_start(&timer, 10000);
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_Pin)) {
		if (!util_old_timer_wait(&timer)) {
			system_error_handler(I2C_ERROR, NULL);
		}
		asm("nop");
	}

	HAL_GPIO_WritePin(I2C_PORT, I2C_SCL_Pin, GPIO_PIN_SET);
	util_old_timer_start(&timer, 10000);
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL_Pin)) {
		if (!util_old_timer_wait(&timer)) {
			system_error_handler(I2C_ERROR, NULL);
		}
		asm("nop");
	}

	GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
//	GPIO_InitStructure.Alternate = GPIO_AF4_I2C1;

	GPIO_InitStructure.Pin = I2C_SCL_Pin;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = I2C_SDA_Pin;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStructure);

	EEPROM_I2C.Instance->CR1 |= 0x8000;
	asm("nop");
	EEPROM_I2C.Instance->CR1 &= (unsigned)~0x8000;
	asm("nop");

	EEPROM_I2C.Instance->CR1 |= 0x0001;

	HAL_I2C_Init(&EEPROM_I2C);
#endif
}

char* get_system_serial_str(void)
{
	uint32_t uid_base = 0x1FFFF7E8;

	uint16_t *idBase0 = (uint16_t*)(uid_base);
	uint16_t *idBase1 = (uint16_t*)(uid_base + 0x02);
	uint32_t *idBase2 = (uint32_t*)(uid_base + 0x04);
	uint32_t *idBase3 = (uint32_t*)(uid_base + 0x08);

	static char str_uid[25] = {0};
	memset((void*)str_uid, 0, sizeof(str_uid));
	sprintf(str_uid, "%04X%04X%08lX%08lX", *idBase0, *idBase1, *idBase2, *idBase3);

	return str_uid;
}
