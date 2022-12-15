#ifndef INC_SIM_MODULE_H_
#define INC_SIM_MODULE_H_

void sim_module_degin();
void sim_module_proccess();
void send_http(const char* url, const char* data);
char* get_response();

#endif
