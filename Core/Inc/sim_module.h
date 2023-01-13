#ifndef INC_SIM_MODULE_H_
#define INC_SIM_MODULE_H_


#include <stdbool.h>


#define END_OF_STRING 0x1a


void sim_module_begin();
void sim_module_proccess();
void sim_proccess_input(const char input_chr);
void connect_to_server();
void send_http(const char* data);
bool is_server_available();
bool is_http_success();
bool is_module_ready();
bool is_http_busy();
char* get_response();

#endif
