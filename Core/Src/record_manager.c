/*
 * data_logger.c
 *
 *  Created on: Dec 15, 2022
 *      Author: RCPark
 */


#include "record_manager.h"

#include <fatfs.h>
#include <string.h>
#include "stm32f1xx_hal.h"

#include "internal_storage.h"
#include "sim_module.h"
#include "settings.h"
#include "logger.h"
#include "utils.h"


#define CLUSTERS_MIN 10


uint32_t _get_free_space();


const char* RECORD_TAG = "RCRD";
const char* RECORD_FILENAME = "log.bin";


uint8_t record_load_ok;


record_status_t next_record_load() {
	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	record_load_ok = 1;

	UINT br = 0;
	UINT ptr = 0;
	char filename[64] = {};
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, RECORD_FILENAME);

	FRESULT res;
do_readline:
	br = 0;
	res = intstor_read_line(filename, &tmpbuf, sizeof(tmpbuf), &br, ptr);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, " read_file(%s) error=%i\n", filename, res);
	}
	if (br == 0) {
		return RECORD_NO_LOG;
	}
	if (!tmpbuf.v1.payload_record.id) {
		ptr += sizeof(tmpbuf);
		goto do_readline;
	}
	if (tmpbuf.v1.payload_record.id <= module_settings.server_log_id) {
		ptr += sizeof(tmpbuf);
		goto do_readline;
	}

	if(tmpbuf.header.magic != RECORD_SD_PAYLOAD_MAGIC) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, " bad record magic %08lX!=%08lX\n", tmpbuf.header.magic, RECORD_SD_PAYLOAD_MAGIC);
	}

	if(tmpbuf.header.version != RECORD_SD_PAYLOAD_VERSION) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, " bad record version %i!=%i\n", tmpbuf.header.version, RECORD_SD_PAYLOAD_VERSION);
	}

	if(!record_load_ok) {
		LOG_DEBUG(RECORD_TAG, " record not loaded\r\n");
		return RECORD_ERROR;
	}

	LOG_DEBUG(RECORD_TAG, " loading record\n");
	record_tag_t** pos = record_cbs;
	while((*pos)) {
		if((*pos)->load_cb) (*pos)->load_cb(&tmpbuf);
		pos++;
	}

	if(!record_load_ok) return RECORD_ERROR;

	return RECORD_OK;
}


record_status_t record_save() {
	if (_get_free_space() < CLUSTERS_MIN) {
		remove_old_records();
	}

	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	record_tag_t** pos = record_cbs;
	while((*pos)) {
		if((*pos)->save_cb) (*pos)->save_cb(&tmpbuf);
		pos++;
	}

	tmpbuf.header.magic = RECORD_SD_PAYLOAD_MAGIC;
	tmpbuf.header.version = RECORD_SD_PAYLOAD_VERSION;

	WORD crc = 0;
	uint32_t tmp = sizeof(tmpbuf.bits);
	for(uint16_t i = 0; i < sizeof(tmpbuf.bits); i++)
		DIO_SPI_CardCRC16(&crc, tmpbuf.bits[i]);
	tmpbuf.crc = crc;

	LOG_DEBUG(RECORD_TAG, " saving record\n");
	Debug_HexDump(RECORD_TAG, (uint8_t*)&tmpbuf, sizeof(tmpbuf));

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, RECORD_FILENAME);

	UINT br;
	FRESULT res = intstor_append_file(filename, &tmpbuf, sizeof(tmpbuf), &br);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, " record NOT saved\n");
		return RECORD_ERROR;
	}

	LOG_DEBUG(RECORD_TAG, " record saved\n");
	return RECORD_OK;
}

record_status_t record_change(uint32_t old_id)
{
	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	// Find line
	record_load_ok = 1;

	UINT br = 0;
	UINT ptr = 0;
	char filename[64] = {};
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, RECORD_FILENAME);

	FRESULT res;
do_readline:
	res = intstor_read_line(filename, &tmpbuf, sizeof(tmpbuf), &br, ptr);
	if (br == 0) {
		LOG_DEBUG(RECORD_TAG, " record not found\r\n");
		goto do_fail;
	}
	if (tmpbuf.v1.payload_record.id != old_id) {
		ptr += sizeof(tmpbuf);
		goto do_readline;
	}

	// Change line
	memset(&tmpbuf, 0, sizeof(tmpbuf));
	record_tag_t** pos = record_cbs;
	while((*pos)) {
		if((*pos)->save_cb) (*pos)->save_cb(&tmpbuf);
		pos++;
	}

	tmpbuf.header.magic = RECORD_SD_PAYLOAD_MAGIC;
	tmpbuf.header.version = RECORD_SD_PAYLOAD_VERSION;

	WORD crc = 0;
	uint32_t tmp = sizeof(tmpbuf.bits);
	for(uint16_t i = 0; i < sizeof(tmpbuf.bits); i++)
		DIO_SPI_CardCRC16(&crc, tmpbuf.bits[i]);
	tmpbuf.crc = crc;

	res = instor_change_file(filename, &tmpbuf, sizeof(tmpbuf), &br, ptr);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, " record NOT saved\n");
		goto do_fail;

	} else {
		LOG_DEBUG(RECORD_TAG, " record changed\n");
		goto do_success;
	}

do_success:

	return RECORD_OK;

do_fail:

	return RECORD_ERROR;
}

uint32_t get_new_id()
{
	LOG_DEBUG(RECORD_TAG, " get new log id\n");
	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	record_load_ok = 1;

	FRESULT res = instor_find_file(RECORD_FILENAME);
	if (res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, " find_file(%s) error=%i\n", RECORD_FILENAME, res);
		goto do_first_id;
	}

	UINT br = 0;
	UINT ptr = DIOSPIFileInfo.fsize - sizeof(record_sd_payload_t);
	char filename[64] = {};
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, RECORD_FILENAME);

	res = intstor_read_line(filename, &tmpbuf, sizeof(tmpbuf), &br, ptr);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, " read_file(%s) error=%i\n", filename, res);
	}
	if (br == 0) {
		goto do_first_id;
	}
	if (!tmpbuf.v1.payload_record.id) {
		goto do_first_id;
	}

	if(tmpbuf.header.magic != RECORD_SD_PAYLOAD_MAGIC) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, " bad record magic %08lX!=%08lX\n", tmpbuf.header.magic, RECORD_SD_PAYLOAD_MAGIC);
	}

	if(tmpbuf.header.version != RECORD_SD_PAYLOAD_VERSION) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, " bad record version %i!=%i\n", tmpbuf.header.version, RECORD_SD_PAYLOAD_VERSION);
	}

	if(!record_load_ok) {
		LOG_DEBUG(RECORD_TAG, " record not loaded\r\n");
		goto do_first_id;
	}

	if(!record_load_ok) {
		goto do_first_id;
	}

	uint32_t new_id = tmpbuf.v1.payload_record.id + 1;
	if (new_id == 0) {
		goto do_first_id;
	}

	LOG_DEBUG(RECORD_TAG, " next record id - %lu\r\n", new_id);

	return new_id;

do_first_id:

	LOG_DEBUG(RECORD_TAG, " set first record id - %d\r\n", FIRST_ID);
	return FIRST_ID;
}

record_status_t remove_old_records()
{
	char filename[64] = {};
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, RECORD_FILENAME);
	if (instor_remove_file(RECORD_FILENAME) == FR_OK) {
		LOG_DEBUG(RECORD_TAG, " file %s removed\r\n", filename);
		return RECORD_OK;
	}
	LOG_DEBUG(RECORD_TAG, " file %s not removed\r\n", filename);
	return RECORD_ERROR;
}

record_status_t record_file_exists()
{
	FRESULT res = instor_find_file(RECORD_FILENAME);
	if (res == FR_NO_FILE) {
		return RECORD_NO_LOG;
	}
	if (res != FR_OK) {
		return RECORD_ERROR;
	}
	return RECORD_OK;
}

uint32_t _get_free_space()
{
	DWORD fre_clust = 0;
	FRESULT res = FR_OK;

	res = instor_get_free_clust(&fre_clust);
	if (res != FR_OK) {
		LOG_DEBUG(RECORD_TAG, " unable to get free space\r\n");
		return 0xFFFFFFFF;
	}

	return fre_clust;
}
