#ifndef INC_SIM_MODULE_H_
#define INC_SIM_MODULE_H_


#include <stdbool.h>


#define END_OF_STRING 0x1a


void sim_module_begin();
void sim_module_proccess();
void sim_proccess_input(const char input_chr);
void send_http(const char* client_tag, const char* data);
bool has_http_response(const char* client_tag);
bool if_http_ready();
bool if_sim_module_busy();
char* get_response();

#endif
