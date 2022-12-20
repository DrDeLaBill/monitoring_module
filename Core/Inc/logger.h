/*
 * record.h
 *
 *  Created on: 19 дек. 2022 г.
 *      Author: georgy
 */

#ifndef INC_LOGGER_H_
#define INC_LOGGER_H_

#include "record_manager.h"

extern record_tag_t general_record_load;
extern LogRecord log_record;

void logger_manager_begin();
void logger_proccess();

#endif /* INC_LOGGER_H_ */
