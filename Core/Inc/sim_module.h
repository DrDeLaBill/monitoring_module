#ifndef INC_SIM_MODULE_H_
#define INC_SIM_MODULE_H_


#include <stdbool.h>


#define RESPONSE_SIZE 360
#define END_OF_STRING 0x1a


extern char response[RESPONSE_SIZE];


void sim_module_begin();
void sim_module_proccess();
void sim_proccess_input(const char input_chr);
void send_http_post(const char* data);
bool has_http_response();
bool if_network_ready();
char* get_response();

#endif
