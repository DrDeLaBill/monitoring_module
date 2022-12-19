/*
 * internal_storage.h
 *
 *  Created on: 2 ���. 2022 �.
 *      Author: gauss
 */

#ifndef INC_INTERNAL_STORAGE_H_
#define INC_INTERNAL_STORAGE_H_

#include "stm32f1xx_hal.h"

#define SD_PAYLOAD_BITS_SIZE(S) ( 			\
	S 										\
	- sizeof(struct _sd_payload_header_t)	\
	- sizeof(uint16_t)						\
)


struct _sd_payload_header_t {
	uint32_t magic; uint8_t version;
};


#include <fatfs.h>
FRESULT intstor_read_file(const char* filename, void* buf, UINT size, UINT* br);
FRESULT intstor_read_line(const char* filename, void* buf, UINT size, UINT* br, UINT ptr);
FRESULT intstor_write_file(const char* filename, const void* buf, UINT size, UINT* bw);
FRESULT intstor_append_file(const char* filename, const void* buf, UINT size, UINT* bw);
FRESULT instor_find_file(const char* pattern);


#endif /* INC_INTERNAL_STORAGE_H_ */
