/*
 * functions.h
 *
 *  Created on: Sep 4, 2022
 *      Author: georg
 */

#ifndef INC_FUNCTIONS_H_
#define INC_FUNCTIONS_H_

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include "settings_manager.h"

extern settings_tag_t general_settings_load;
extern ModuleSettings module_settings;

bool set_clock_initialized(bool value);
void show_settings();
int _write(int file, uint8_t *ptr, int len);

#endif /* INC_FUNCTIONS_H_ */
