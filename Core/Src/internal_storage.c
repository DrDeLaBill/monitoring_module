/*
 * internal_storage.c
 *
 *  Created on: 2 ���. 2022 �.
 *      Author: gauss
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "stm32f1xx_hal.h"
#include "fatfs.h"
//
#include "utils.h"
#include "user_diskio_spi.h"
#include "internal_storage.h"


const char* STOR_MODULE_TAG = "STOR";


FRESULT intstor_read_file(const char* filename, void* buf, UINT size, UINT* br) {
	return intstor_read_line(filename, buf, size, br, 0);
}

FRESULT intstor_read_line(const char* filename, void* buf, UINT size, UINT* br, UINT ptr) {
	FRESULT res;
	FRESULT out = FR_OK;

	if(br) (*br) = 0;

	res = f_mount(&DIOSPIFatFS, DIOSPIPath, 1);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount() error=%i\n", res);
		return res;
	}

	res = f_open(&DIOSPIFile, filename, FA_OPEN_EXISTING|FA_READ);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_open() error=%i\r\n", res);
		out = res;
		goto do_umount;
	}

	res = f_lseek(&DIOSPIFile, ptr);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_lseek() error=%i\n", res);
		out = res;
		goto do_close;
	}

	res = f_read(&DIOSPIFile, (uint8_t*)buf, size, br);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_read() error=%i\n", res);
		out = res;
		goto do_close;
	}

do_close:
	res = f_close(&DIOSPIFile);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_close() error=%i\n", res);
	}

do_umount:
	res = f_mount(NULL, DIOSPIPath, 0);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount(umount) error=%i\n", res);
	}

	return out;
}

FRESULT intstor_write_file(const char* filename, const void* buf, UINT size, UINT* bw) {
	FRESULT res;
	FRESULT out = FR_OK;

	if(bw) (*bw) = 0;

	res = f_mount(&DIOSPIFatFS, DIOSPIPath, 1);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount() error=%i\n", res);
		return res;
	}

	res = f_open(&DIOSPIFile, filename, FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_open() error=%i\n", res);
		out = res;
		goto do_umount;
	}

	res = f_write(&DIOSPIFile, (uint8_t*)buf, size, bw);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_write() error=%i\n", res);
		out = res;
		goto do_close;
	}

do_close:
	res = f_close(&DIOSPIFile);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_close() error=%i\n", res);
	}

do_umount:
	res = f_mount(NULL, DIOSPIPath, 0);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount(umount) error=%i\n", res);
	}

	return out;
}


FRESULT intstor_append_file(const char* filename, const void* buf, UINT size, UINT* bw)
{
	FRESULT res = instor_find_file(filename);
	UINT file_size = 0;
	if(res == FR_OK) {
		file_size = f_size(&DIOSPIFile);
	}
	return instor_change_file(filename, buf, size, bw, file_size);
}


FRESULT instor_change_file(const char* filename, const void* buf, UINT size, UINT* bw, UINT ptr)
{
	FRESULT res;
	FRESULT out = FR_OK;

	if(bw) (*bw) = 0;

	res = f_mount(&DIOSPIFatFS, DIOSPIPath, 1);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount() error=%i\n", res);
		return res;
	}

	res = f_open(&DIOSPIFile, filename, FA_OPEN_ALWAYS | FA_WRITE);
	if (res == FR_NO_FILE) {
		res = f_open(&DIOSPIFile, filename, FA_CREATE_ALWAYS | FA_WRITE);
	}
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_open() error=%i\n", res);
		out = res;
		goto do_umount;
	}

	res = f_lseek(&DIOSPIFile, ptr);
	if (res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_lseek() error=%i\n", res);
		out = res;
		goto do_umount;
	}

	res = f_write(&DIOSPIFile, (uint8_t*)buf, size, bw);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_write() error=%i\n", res);
		out = res;
		goto do_close;
	}

do_close:
	res = f_close(&DIOSPIFile);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_close() error=%i\n", res);
	}

do_umount:
	res = f_mount(NULL, DIOSPIPath, 0);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount(umount) error=%i\n", res);
	}

	return out;
}


FRESULT instor_remove_file(const char* filename)
{
	FRESULT res;
	FRESULT out = FR_OK;

	res = f_mount(&DIOSPIFatFS, DIOSPIPath, 1);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount() error=%i\n", res);
		return res;
	}

	res = f_unlink(filename);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_unlink(umount) error=%i\n", res);
		out = res;
		goto do_umount;
	}

do_umount:
	res = f_mount(NULL, DIOSPIPath, 0);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount(umount) error=%i\n", res);
	}

	return out;
}


FRESULT instor_find_file(const char* pattern)
{
	FRESULT res;
	FRESULT out = FR_OK;
	DIR dr;

	res = f_mount(&DIOSPIFatFS, DIOSPIPath, 1);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount() error=%i\n", res);
		return res;
	}

	res = f_findfirst(&dr, &DIOSPIFileInfo, DIOSPIPath, pattern);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_findfirst(umount) error=%i\n", res);
		out = res;
		goto do_umount;
	}

do_umount:
	res = f_mount(NULL, DIOSPIPath, 0);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount(umount) error=%i\n", res);
	}

	return out;
}

FRESULT instor_get_free_clust(DWORD *fre_clust)
{
	FRESULT res;
	FRESULT out = FR_OK;

	res = f_mount(&DIOSPIFatFS, DIOSPIPath, 1);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount() error=%i\n", res);
		return res;
	}

	FATFS *dio_spi_fatfs_ptr = &DIOSPIFatFS;
	res = f_getfree((TCHAR*)DIOSPIPath, fre_clust, &dio_spi_fatfs_ptr);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_getfree(umount) error=%i\n", res);
		out = res;
		goto do_umount;
	}

do_umount:
	res = f_mount(NULL, DIOSPIPath, 0);
	if(res != FR_OK) {
		LOG_DEBUG(STOR_MODULE_TAG, "f_mount(umount) error=%i\n", res);
	}

	return out;
}
