#ifndef INC_SIM_MODULE_H_
#define INC_SIM_MODULE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>


#ifdef DEBUG
#   define SIM_MODULE_DEBUG (1)
#endif


#define RESPONSE_SIZE (430)
#define END_OF_STRING (0x1a)
#define SIM_LOG_SIZE  (200)


extern char sim_response[RESPONSE_SIZE];


void sim_begin();
void sim_proccess();
void sim_proccess_input(const char input_chr);
void send_sim_http_post(const char* data);
bool has_http_response();
bool if_network_ready();
char* get_response();
char* get_sim_url();


#ifdef __cplusplus
}
#endif


#endif
