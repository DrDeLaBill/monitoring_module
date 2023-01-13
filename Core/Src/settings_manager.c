/*
 * settings_manager.c
 *
 *  Created on: May 27, 2022
 *      Author: gauss
 */

#include "stm32f1xx_hal.h"

#include <fatfs.h>
#include <string.h>
//
#include "utils.h"
//
#include "internal_storage.h"
#include "settings_manager.h"


const char* SETMGR_MODULE_TAG = "SETMGR";


uint8_t settings_load_ok;


const char* settings_filename = "settings.bin";


void settings_reset() {
	settings_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));
	settings_tag_t** pos;

	pos = settings_cbs;
	while((*pos)) {
		if((*pos)->default_cb) (*pos)->default_cb(&tmpbuf);
		pos++;
	}
	pos = settings_cbs;
	while((*pos)) {
		if((*pos)->load_cb) (*pos)->load_cb(&tmpbuf);
		pos++;
	}
}


settings_status_t settings_load() {
	settings_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, settings_filename);

	UINT br;
	settings_load_ok = 1;
	FRESULT res = intstor_read_file(filename, &tmpbuf, sizeof(tmpbuf), &br);
	if(res != FR_OK) {
		settings_load_ok = 0;
		LOG_DEBUG(SETMGR_MODULE_TAG, "read_file(%s) error=%i\n", filename, res);
	}

	if(tmpbuf.header.magic != SETTINGS_SD_PAYLOAD_MAGIC) {
		settings_load_ok = 0;
		LOG_DEBUG(SETMGR_MODULE_TAG, "bad settings magic %08lX!=%08lX\n", tmpbuf.header.magic, SETTINGS_SD_PAYLOAD_MAGIC);
	}

	if(tmpbuf.header.version != SETTINGS_SD_PAYLOAD_VERSION) {
		settings_load_ok = 0;
		LOG_DEBUG(SETMGR_MODULE_TAG, "bad settings version %i!=%i\n", tmpbuf.header.version, SETTINGS_SD_PAYLOAD_VERSION);
	}

	if(!settings_load_ok) {
		LOG_DEBUG(SETMGR_MODULE_TAG, "settings not loaded, using defaults\n");
		settings_tag_t** pos = settings_cbs;
		while((*pos)) {
			if((*pos)->default_cb) (*pos)->default_cb(&tmpbuf);
			pos++;
		}
	}

	LOG_DEBUG(SETMGR_MODULE_TAG, "applying settings\n");
	settings_tag_t** pos = settings_cbs;
	while((*pos)) {
		if((*pos)->load_cb) (*pos)->load_cb(&tmpbuf);
		pos++;
	}

	if(!settings_load_ok) return settings_save();

	return SETTINGS_OK;
}


settings_status_t settings_save() {
	settings_sd_payload_t tmpbuf;
	memset(&tmpbuf, 0, sizeof(tmpbuf));

	settings_tag_t** pos = settings_cbs;
	while((*pos)) {
		if((*pos)->save_cb) (*pos)->save_cb(&tmpbuf);
		pos++;
	}

	tmpbuf.header.magic = SETTINGS_SD_PAYLOAD_MAGIC;
	tmpbuf.header.version = SETTINGS_SD_PAYLOAD_VERSION;

	WORD crc = 0;
	for(uint16_t i = 0; i < sizeof(tmpbuf.bits); i++)
		DIO_SPI_CardCRC16(&crc, tmpbuf.bits[i]);
	tmpbuf.crc = crc;

	LOG_DEBUG(SETMGR_MODULE_TAG, "saving settings\n");
	Debug_HexDump(SETMGR_MODULE_TAG, (uint8_t*)&tmpbuf, sizeof(tmpbuf));

	char filename[64];
	snprintf(filename, sizeof(filename), "%s" "%s", DIOSPIPath, settings_filename);

	UINT br;
	FRESULT res = intstor_write_file(filename, &tmpbuf, sizeof(tmpbuf), &br);
	if(res != FR_OK) {
		settings_load_ok = 0;
		LOG_DEBUG(SETMGR_MODULE_TAG, "settings NOT saved\n");
		return SETTINGS_ERROR;

	} else {
		LOG_DEBUG(SETMGR_MODULE_TAG, "settings saved\n");
		return SETTINGS_OK;

	}
}

