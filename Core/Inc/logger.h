/*
 * record.h
 *
 *  Created on: 19 дек. 2022 г.
 *      Author: georgy
 */

#ifndef INC_LOGGER_H_
#define INC_LOGGER_H_

#include "record_manager.h"


#define LOG_SIZE 350


extern record_tag_t general_record_load;
extern LogRecord log_record;


void logger_manager_begin();
void logger_proccess();
void update_log_sleep(uint32_t time)
void clear_log();


#endif /* INC_LOGGER_H_ */
