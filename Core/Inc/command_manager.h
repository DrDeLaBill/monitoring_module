/*
 * uart_command.h
 *
 *  Created on: Sep 5, 2022
 *      Author: georg
 */

#ifndef INC_COMMAND_MANAGER_H_
#define INC_COMMAND_MANAGER_H_

#include <stdbool.h>

#define CHAR_COMMAND_SIZE 80
#define UART_RESPONSE_SIZE 40

void command_manager_begin();

#endif /* INC_COMMAND_MANAGER_H_ */
