/*
 * user_diskio_spi.h
 *
 *  Created on: Apr 27, 2022
 *      Author: gauss
 */

#ifndef TARGET_USER_DISKIO_SPI_H_
#define TARGET_USER_DISKIO_SPI_H_


#include "integer.h"
#include "diskio.h"
#include "ff_gen_drv.h"

extern Diskio_drvTypeDef DIO_SPI_Driver;


//#define DIO_SPI_DEBUG
//#define DIO_SPI_CMD_DEBUG


BYTE DIO_SPI_CardCRC7(BYTE* pcrc, BYTE b);
WORD DIO_SPI_CardCRC16(WORD* pcrc, BYTE b);


DSTATUS DIO_SPI_initialize(BYTE pdrv);
DRESULT DIO_SPI_read(BYTE pdrv, BYTE* buf, DWORD sector, UINT count);
DRESULT DIO_SPI_ioctl(BYTE pdrv, BYTE cmd, void* buf);
DSTATUS DIO_SPI_status(BYTE pdrv);

#if _USE_WRITE
DRESULT DIO_SPI_write(BYTE pdrv, const BYTE* buf, DWORD sector, UINT count);
#endif


typedef void (*_DIO_SPI_Delay_cb_t)(uint32_t ms);
extern _DIO_SPI_Delay_cb_t DIO_SPI_Delay_cb;


#endif /* TARGET_USER_DISKIO_SPI_H_ */
