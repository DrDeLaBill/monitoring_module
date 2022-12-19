/*
 * record.c
 *
 *  Created on: 19 дек. 2022 г.
 *      Author: georgy
 */

record_tag_t general_record_load = {
	.save_cb = &_general_record_save,
	.load_cb = &_general_record_load
};
record_tag_t* record_cbs[] = {
	&general_record_load,
	NULL
};
