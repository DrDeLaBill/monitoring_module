/*
 * uart_command.h
 *
 *  Created on: Sep 5, 2022
 *      Author: georg
 */

#ifndef INC_COMMAND_MANAGER_H_
#define INC_COMMAND_MANAGER_H_

#include <stdbool.h>

#define CHAR_COMMAND_SIZE  40
#define UART_RESPONSE_SIZE 40

void command_manager_begin();
void command_manager_proccess();
void cmd_proccess_input(const char input_chr);

#endif /* INC_COMMAND_MANAGER_H_ */
