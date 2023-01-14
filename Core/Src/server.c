/*
 * server.c
 *
 *  Created on: Jan 14, 2023
 *      Author: georgy
 */

#include "server.h"

#include "sim_module.h"


void server_proccess()
{
	if (is_module_ready() && !is_server_available()) {
		connect_to_server();
	}
}
