/*
 * data_logger.c
 *
 *  Created on: Dec 15, 2022
 *      Author: RCPark
 */


#include <log_manager.h>
#include "stm32f1xx_hal.h"

#include <fatfs.h>
#include "sim_module.h"
#include "settings.h"
#include "utils.h"

#define LOG_SIZE 250


void _write_log();
void _remove_old_log();
void _get_old_log();
void _check_and_remove(const char* response);


const char* LOG_TAG = "LOG";

const char* filename_pattern = "*_log.bin";

dio_timer_t log_timer;


void logger_begin()
{
	Util_TimerStart(&log_timer, module_settings.sleep_time);
}

void logger_proccess()
{
	if (is_module_ready() && !is_server_available()) {
		connect_to_server();
	}

	if (is_server_available()) {
		char data[LOG_SIZE] = {};
		snprintf(
			data,
			"pussy",
			12
		);
		send_http(data);
	}

	if (is_http_success()) {
		LOG_DEBUG(LOG_TAG, "%s\r\n", get_response());
		_check_and_remove(get_response());
	}

	if (Util_TimerPending(&log_timer)) {
		return;
	}

	_remove_old_log();
	_write_log();
}

record_status_t first_record_load() {
	record_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	record_load_ok = 1;

	char filename[64];
	FRESULT res = instor_find_file(filename_pattern);
	if (res == FR_OK) {
		snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, DIOSPIFileInfo.lfname);
	}
    if (res == FR_NO_FILE) {
		snprintf(
			filename,
			sizeof(filename),
			"%s" "%d_%d_%d_log.bin",
			DIOSPIPath,
			DS1307_GetYear(),
			DS1307_GetMonth(),
			DS1307_GetDate()
		);
	}
    if (res != FR_OK && res != FR_NO_FILE) {
    	record_load_ok = 0;
		LOG_DEBUG(LOG_TAG, "find_file(%s) error=%i\n", filename_pattern, res);
    }

	UINT br;
	UINT ptr = 0;

do_readline:
	res = intstor_read_line(filename, &tmpbuf, sizeof(tmpbuf), &br, ptr);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(LOG_TAG, "read_file(%s) error=%i\n", filename, res);
	}
	if (!tmpbuf.v1.payload_record.id) {
		goto do_readline;
	}

	if(tmpbuf.header.magic != RECORD_SD_PAYLOAD_MAGIC) {
		record_load_ok = 0;
		LOG_DEBUG(LOG_TAG, "bad record magic %08lX!=%08lX\n", tmpbuf.header.magic, RECORD_SD_PAYLOAD_MAGIC);
	}

	if(tmpbuf.header.version != RECORD_SD_PAYLOAD_VERSION) {
		record_load_ok = 0;
		LOG_DEBUG(LOG_TAG, "bad record version %i!=%i\n", tmpbuf.header.version, RECORD_SD_PAYLOAD_VERSION);
	}

	if(!record_load_ok) {
		LOG_DEBUG(LOG_TAG, "record not loaded\r\n");
		return RECORD_ERROR;
	}

	LOG_DEBUG(LOG_TAG, "loading record\n");
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

	LOG_DEBUG(LOG_TAG, "saving record\n");
	Debug_HexDump(LOG_TAG, (uint8_t*)&tmpbuf, sizeof(tmpbuf));

	char filename[64];
	snprintf(
		filename,
		sizeof(filename),
		"%s" "%d_%d_%d_log.bin",
		DIOSPIPath,
		DS1307_GetYear(),
		DS1307_GetMonth(),
		DS1307_GetDate()
	);

	UINT br;
	FRESULT res = intstor_append_file(filename, &tmpbuf, sizeof(tmpbuf), &br);
	if(res != FR_OK) {
		record_load_ok = 0;
		LOG_DEBUG(LOG_TAG, "record NOT saved\n");
		return SETTINGS_ERROR;

	} else {
		LOG_DEBUG(LOG_TAG, "record saved\n");
		return SETTINGS_OK;
	}
}

void _check_and_remove(const char* response)
{

}

void _write_log()
{

}

void _remove_old_log()
{

}

void _get_old_log()
{

}


