/*
 * logger.h
 *
 *  Created on: 19 дек. 2022 г.
 *      Author: DrDeLaBill
 */

#ifndef INC_LOGGER_H_
#define INC_LOGGER_H_

#include "record_manager.h"


#define LOGGER_DEBUG (false)

#define LOG_SIZE 370


void logger_manager_begin();
void logger_proccess();
void update_log_timer();
void log_update_sleep(uint32_t time);
void clear_log();


#endif /* INC_LOGGER_H_ */
