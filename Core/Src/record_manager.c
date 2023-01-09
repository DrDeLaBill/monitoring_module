/*
 * data_logger.c
 *
 *  Created on: Dec 15, 2022
 *      Author: RCPark
 */


#include "stm32f1xx_hal.h"

#include <fatfs.h>
#include <record_manager.h>
#include "sim_module.h"
#include "settings.h"
#include "logger.h"
#include "utils.h"


#define FIRST_ID 1


const char* RECORD_TAG = "RCRD";
const char* record_filename = "log.bin";


record_status_t first_record_load() {
	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	record_load_ok = 1;

	UINT br;
	UINT ptr = 0;
	char filename[64] = {};
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, record_filename);

	FRESULT res;
do_readline:
	res = intstor_read_line(filename, &tmpbuf, sizeof(tmpbuf), &br, ptr);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, "read_file(%s) error=%i\n", filename, res);
	}
	if (br == 0) {
		return RECORD_ERROR;
	}
	if (!tmpbuf.v1.payload_record.id) {
		ptr += sizeof(tmpbuf);
		goto do_readline;
	}

	if(tmpbuf.header.magic != RECORD_SD_PAYLOAD_MAGIC) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, "bad record magic %08lX!=%08lX\n", tmpbuf.header.magic, RECORD_SD_PAYLOAD_MAGIC);
	}

	if(tmpbuf.header.version != RECORD_SD_PAYLOAD_VERSION) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, "bad record version %i!=%i\n", tmpbuf.header.version, RECORD_SD_PAYLOAD_VERSION);
	}

	if(!record_load_ok) {
		LOG_DEBUG(RECORD_TAG, "record not loaded\r\n");
		return RECORD_ERROR;
	}

	LOG_DEBUG(RECORD_TAG, "loading record\n");
	record_tag_t** pos = record_cbs;
	while((*pos)) {
		if((*pos)->load_cb) (*pos)->load_cb(&tmpbuf);
		pos++;
	}

	if(!record_load_ok) return RECORD_ERROR;

	return RECORD_OK;
}


record_status_t record_save() {
	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	record_tag_t** pos = settings_cbs;
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

	LOG_DEBUG(RECORD_TAG, "saving record\n");
	Debug_HexDump(RECORD_TAG, (uint8_t*)&tmpbuf, sizeof(tmpbuf));

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", record_filename);

	UINT br;
	FRESULT res = intstor_append_file(filename, &tmpbuf, sizeof(tmpbuf), &br);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, "record NOT saved\n");
		return SETTINGS_ERROR;

	} else {
		LOG_DEBUG(RECORD_TAG, "record saved\n");
		return SETTINGS_OK;
	}
}

record_status_t record_change(uint32_t id, record_sd_payload_t *payload_change)
{
	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	record_load_ok = 1;

	UINT br;
	UINT ptr = 0;
	char filename[64] = {};
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, record_filename);

	FRESULT res;
do_readline:
	res = intstor_read_line(filename, &tmpbuf, sizeof(tmpbuf), &br, ptr);
	if (br == 0) {
		LOG_DEBUG(RECORD_TAG, "record not found\r\n");
		goto do_fail;
	}
	if (tmpbuf.v1.payload_record.id != id) {
		ptr += sizeof(tmpbuf);
		goto do_readline;
	}

	res = intstor_change_file(filename, &payload_change, sizeof(payload_change), &br, ptr);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, "record NOT saved\n");
		goto do_fail;

	} else {
		LOG_DEBUG(RECORD_TAG, "record changed\n");
		goto do_success;
	}

do_success:

	return RECORD_OK;

do_fail:

	return RECORD_ERROR;
}

uint32_t get_new_id()
{
	// искать максимальный ID, а не последний
	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	record_load_ok = 1;

	FRESULT res = instor_find_file(record_filename);
	if (res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, "find_file(%s) error=%i\n", record_filename, res);
		return FIRST_ID;
	}

	UINT br;
	UINT ptr = DIOSPIFileInfo.fsize - sizeof(record_sd_payload_t);
	char filename[64] = {};
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, record_filename);

	res = intstor_read_line(filename, &tmpbuf, sizeof(tmpbuf), &br, ptr);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, "read_file(%s) error=%i\n", filename, res);
	}
	if (br == 0) {
		goto do_first_id;
	}
	if (!tmpbuf.v1.payload_record.id) {
		goto do_first_id;
	}

	if(tmpbuf.header.magic != RECORD_SD_PAYLOAD_MAGIC) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, "bad record magic %08lX!=%08lX\n", tmpbuf.header.magic, RECORD_SD_PAYLOAD_MAGIC);
	}

	if(tmpbuf.header.version != RECORD_SD_PAYLOAD_VERSION) {
		record_load_ok = 0;
		LOG_DEBUG(RECORD_TAG, "bad record version %i!=%i\n", tmpbuf.header.version, RECORD_SD_PAYLOAD_VERSION);
	}

	if(!record_load_ok) {
		LOG_DEBUG(RECORD_TAG, "record not loaded\r\n");
		return RECORD_ERROR;
	}

	LOG_DEBUG(RECORD_TAG, "loading record\n");
	record_tag_t** pos = record_cbs;
	while((*pos)) {
		if((*pos)->load_cb) (*pos)->load_cb(&tmpbuf);
		pos++;
	}

	if(!record_load_ok) goto do_first_id;

	return log_record.id + 1;

do_first_id:

	return FIRST_ID;
}

record_status_t remove_old_records(uint32_t last_id)
{

}