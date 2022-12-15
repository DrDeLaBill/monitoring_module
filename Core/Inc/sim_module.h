#ifndef INC_SIM_MODULE_H_
#define INC_SIM_MODULE_H_

#include "stm32f1xx_hal.h"
#include <stdbool.h>

#define SIM_MODULE_MODEL "SIM5320"
#define RESET_DURATION 100
#define RESET_DELAY 60000
#define DEFAULT_AT_TIMEOUT 700
#define LONG_AT_TIMEOUT 7000
#define DEFAULT_HTTP_TIMEOUT 10000
#define RESP_BUFFER_SIZE 100
#define RESP_BUFFER_HTTP_SIZE 900
#define REQUEST_COMMAND_SIZE 1000
#define NUMBER_OF_ACT_ATTEMPTS 5
#define END_OF_STRING 0x1a

void sim_module_degin(UART_HandleTypeDef *sim_huart, GPIO_TypeDef* sim_reset_GPIO_port, uint16_t sim_reset_pin);
bool sim_module_init();
bool send_log();
bool send_old_log();
char* send_http_GET(char* url);
bool send_http_POST(char* url, char* data);
void restart_sim_module();
bool is_sim_module_ready();
bool is_GPRS_ready();
bool is_sim_module_initialized();
bool is_server_available();

#endif
