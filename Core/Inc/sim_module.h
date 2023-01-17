#ifndef INC_SIM_MODULE_H_
#define INC_SIM_MODULE_H_


#include <stdbool.h>


#define END_OF_STRING 0x1a


void sim_module_begin();
void sim_module_proccess();
void sim_proccess_input(const char input_chr);
void send_http(const char* data);
bool has_http_response();
bool if_network_ready();
char* get_response();

#endif
