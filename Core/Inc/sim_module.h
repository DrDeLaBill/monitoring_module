#ifndef INC_SIM_MODULE_H_
#define INC_SIM_MODULE_H_

#include "stm32f1xx_hal.h"
#include <stdbool.h>

#define SIM_MODULE_MODEL "SIM5320"
//#define RESET_DURATION 100
//#define RESET_DELAY 60000
//#define DEFAULT_AT_TIMEOUT 700
//#define LONG_AT_TIMEOUT 7000
//#define DEFAULT_HTTP_TIMEOUT 10000
//#define RESP_BUFFER_SIZE 100
//#define RESP_BUFFER_HTTP_SIZE 900
//#define REQUEST_COMMAND_SIZE 1000
//#define NUMBER_OF_ACT_ATTEMPTS 5

void sim_module_degin();
void sim_module_proccess();
void send_http(const char* url, const char* data);
char* get_response();

#endif
