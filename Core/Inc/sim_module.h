#ifndef INC_SIM_MODULE_H_
#define INC_SIM_MODULE_H_


#include <stdbool.h>


void sim_module_degin();
void sim_module_proccess();
void connect_to_server();
void send_http(const char* data);
bool is_server_available();
bool is_http_success();
bool is_module_ready();
char* get_response();

#endif
