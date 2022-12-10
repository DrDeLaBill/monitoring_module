/*
 * user_diskio_spi.c
 *
 *  Created on: Apr 27, 2022
 *      Author: gauss
 */

#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "user_diskio_spi.h"
#include "utils.h"


const char* DIOSD_MODULE_TAG = "DIO_SPISD";


#ifdef DIO_SPI_DEBUG
#define DIO_SPI_PRINTF_TAG(fmt, ...) { LOG_DEBUG(DIOSD_MODULE_TAG, fmt __VA_OPT__(,) __VA_ARGS__); }
#define DIO_SPI_PRINTF(fmt, ...) { LOG_DEBUG_LN(fmt __VA_OPT__(,) __VA_ARGS__); }
#else /* DIO_SPI_DEBUG */
#define DIO_SPI_PRINTF_TAG(fmt, ...) {}
#define DIO_SPI_PRINTF(fmt, ...) {}
#endif /* DIO_SPI_DEBUG */


#ifdef DIO_SPI_CMD_DEBUG
#define DIO_SPI_CMD_PRINTF_TAG(fmt, ...) {DIO_SPI_PRINTF_TAG(fmt __VA_OPT__(,) __VA_ARGS__);}
#define DIO_SPI_CMD_PRINTF(fmt, ...) {DIO_SPI_PRINTF(fmt __VA_OPT__(,) __VA_ARGS__);}
#else /* DIO_SPI_CMD_DEBUG */
#define DIO_SPI_CMD_PRINTF(fmt, ...) {}
#define DIO_SPI_CMD_PRINTF_TAG(fmt, ...) {}
#endif /* DIO_SPI_CMD_DEBUG */



// card commands
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define CMD13   (13)		/* SD_STATUS */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */

#define CMD16	(16)		/* SET_BLOCKLEN */

#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */

#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */

#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */

#define CMD55	(55)		/* APP_CMD */

#define CMD58	(58)		/* READ_OCR */
#define CMD59   (59)		/* CRC_ON_OFF */


// R1 result bits
#define RES_R1_IN_IDLE_STATE	(0x01)
#define RES_R1_ERASE_RESET	   	(0x02)
#define RES_R1_ILLEGAL_COMMAND  (0x04)
#define RES_R1_COM_CRC_ERROR	(0x08)
#define RES_R1_ERASE_SEQ_ERROR	(0x10)
#define RES_R1_ADDRESS_ERROR	(0x20)
#define RES_R1_PARAMETER_ERROR	(0x40)
#define RES_R1_FAILED			(0x80)

#define R1_IS_ERROR(res) (res & 0xFE)



// R2 result bits

#define RES_R2_CARD_IS_LOCKED   (0x01)
#define RES_R2_WP_ERASE_SKIP	(0x02)
#define RES_R2_ERROR			(0x04)
#define RES_R2_CC_ERROR			(0x08)
#define RES_R2_CARD_ECC_FAILED  (0x10)
#define RES_R2_WP_VIOLATION		(0x20)
#define RES_R2_ERASE_PARAM		(0x40)
#define RES_R2_OUT_OF_RANGE		(0x80)



// Card type flags
#define CT_MMC		0x01			/* MMC ver 3 */
#define CT_SD1		0x02			/* SD ver 1 */
#define CT_SD2		0x04			/* SD ver 2 */
#define CT_SDC		(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK	0x08			/* Block addressing */


#define CSD_STRUCTURE_VER1 0b00
#define CSD_STRUCTURE_VER2 0b01
#define CSD_STRUCTURE_VER3 0b10


typedef union _csd_reg_t{
	BYTE bits[16];
	struct _common {
		// bits[0]
		uint8_t _rsvd1:6;
		uint8_t CSD_STRUCTURE:2;

	} common;
	struct _v1 {
		// bits[0]
		uint8_t _rsvd1:6;
		uint8_t CSD_STRUCTURE:2;
		// bits[1]
		uint8_t TAAC;
		uint8_t NSAC;
		uint8_t TRAN_SPEED;
		uint8_t CCC_HIGH;
		// bits[5]
		uint8_t READ_BL_LEN:4;
		uint8_t CC_LOW:4;
		// bits[6]
		uint8_t C_SIZE_HIGH:2;
		uint8_t _rsvd2:2;
		uint8_t DSR_IMP:1;
		uint8_t READ_BLK_MISALGN:1;
		uint8_t WRITE_BLK_MISALGN:1;
		uint8_t READ_BLK_PARTIAL:1;
		// bits[7]
		uint8_t C_SIZE_MID;
		// bits[8]
		uint8_t VDD_R_CURR_MAX:3;
		uint8_t VDD_R_CURR_MIN:3;
		uint8_t C_SIZE_LOW:2;
		// bits[9]
		uint8_t C_SIZE_MULT_HIGH:2;
		uint8_t VDD_W_CURR_MAX:3;
		uint8_t VDD_W_CURR_MIN:3;
		// bits[10]
		uint8_t SECTOR_SIZE_HIGH:6;
		uint8_t ERASE_BLK_EN:1;
		uint8_t C_SIZE_MULT_LOW:1;
		// bits[11]
		uint8_t WP_GRP_SIZE:7;
		uint8_t SECTOR_SIZE_LOW:1;
		// bits[12]
		uint8_t WRITE_BL_LEN_HIGH:2;
		uint8_t R2W_FACTOR:3;
		uint8_t _rsvd3:2;
		uint8_t WP_GRP_ENABLE:1;
		// bits[13]
		uint8_t _rsvd4:5;
		uint8_t WRITE_PARTIAL:1;
		uint8_t WRITE_BL_LEN_LOW:2;
		// bits[14]
		uint8_t _rsvd5:2;
		uint8_t FILE_FORMAT:2;
		uint8_t TMP_WRITE_PROTECT:1;
		uint8_t PERM_WRITE_PROTECT:1;
		uint8_t COPY:1;
		uint8_t FILE_FORMAT_GRP:1;
		// bits[15]
		uint8_t _always1:1;
		uint8_t crc:7;
	} v1;

	struct _v2 {
		// bits[0]
        uint8_t _rsvd1:6;
        uint8_t CSD_STRUCTURE:2;

        // bits[1]
        uint8_t TAAC;
        uint8_t NSAC;
        uint8_t TRAN_SPEED;
        uint8_t CCC_HIGH;
        // bits[5]
        uint8_t READ_BL_LEN:4;
        uint8_t CCC_LOW:4;
        // bits[6]
        uint8_t _rsvd2:4;
        uint8_t DSR_IMP:1;
        uint8_t READ_BLK_MISALIGN:1;
        uint8_t WRITE_BLK_MISALIGN:1;
        uint8_t READ_BL_PARTIAL:1;
        // bits[7]
        uint8_t _rsvd3:2;
        uint16_t C_SIZE_HIGH:6;
        // bits[8]
        uint8_t C_SIZE_MID;
        uint8_t C_SIZE_LOW;
        // bits[10]
        uint8_t SECTOR_SIZE_HIGH:6;
        uint8_t ERASE_BLK_EN:1;
        uint8_t _rsvd4:1;
        // bits[11]
        uint8_t WP_GRP_SIZE:7;
        uint8_t SECTOR_SIZE_LOW:1;
        // bits[12]
        uint8_t WRITE_BL_LEN_HIGH:2;
        uint8_t R2W_FACTOR:3;
        uint8_t _rsvd5:2;
        uint8_t WP_GRP_ENABLE:1;
        // bits[13]
        uint8_t _rsvd6:5;
        uint8_t WRITE_BL_PARTIAL:1;
        uint8_t WRITE_BL_LEN_LOW:2;
        // bits[14]
        uint8_t _rsvd7:2;
        uint8_t FILE_FORMAT:2;
        uint8_t TMP_WRITE_PROTECT:1;
        uint8_t PERM_WRITE_PROTECT:1;
        uint8_t COPY:1;
        uint8_t FILE_FORMAT_GRP:1;
        // bits[15]
        uint8_t _always1:1;
        uint8_t crc:7;
	} v2;
} csd_reg_t;



uint32_t csd_c_size_mult(const csd_reg_t* s) {
	switch(s->common.CSD_STRUCTURE) {
	case CSD_STRUCTURE_VER1:
		return (1 << (((s->v1.C_SIZE_MULT_HIGH << 1) | (s->v1.C_SIZE_MULT_LOW)) + 2));

	default:
		return 512;

	}
}


uint32_t csd_c_size(const csd_reg_t* s) {
	switch(s->common.CSD_STRUCTURE) {
	case CSD_STRUCTURE_VER1:
		return (s->v1.C_SIZE_HIGH << 10) | (s->v1.C_SIZE_MID << 2) | (s->v1.C_SIZE_LOW);

	case CSD_STRUCTURE_VER2:
		return (s->v2.C_SIZE_HIGH << 16) | (s->v2.C_SIZE_MID << 8) | (s->v2.C_SIZE_LOW);

	default:
		return 0;
	}

}


uint32_t csd_read_bl_len(const csd_reg_t* s) {
	switch(s->common.CSD_STRUCTURE) {
	case CSD_STRUCTURE_VER1:
		return (1<<s->v1.READ_BL_LEN);

	case CSD_STRUCTURE_VER2:

	default:
		return 0;
	}
}


uint32_t csd_write_bl_len(const csd_reg_t* s) {
	switch(s->common.CSD_STRUCTURE){
	case CSD_STRUCTURE_VER1:
		return (1 << ((s->v1.WRITE_BL_LEN_HIGH << 2) | (s->v1.WRITE_BL_LEN_LOW)));

	case CSD_STRUCTURE_VER2:
		return (1 << ((s->v2.WRITE_BL_LEN_HIGH << 2) | (s->v2.WRITE_BL_LEN_LOW)));

	default:
		return 0;
	}
}


uint32_t csd_sector_size(const csd_reg_t* s) {
	switch(s->common.CSD_STRUCTURE) {
	case CSD_STRUCTURE_VER1:
		return ((s->v1.SECTOR_SIZE_HIGH << 1) | (s->v1.SECTOR_SIZE_LOW)) + 1;

	case CSD_STRUCTURE_VER2:
		return ((s->v2.SECTOR_SIZE_HIGH << 1) | (s->v2.SECTOR_SIZE_LOW)) + 1;

	default:
		return 0;
	}
}


uint8_t csd_erase_block_enabled(const csd_reg_t* s) {
	switch(s->common.CSD_STRUCTURE) {
	case CSD_STRUCTURE_VER1:
		return s->v1.ERASE_BLK_EN;

	case CSD_STRUCTURE_VER2:
		return s->v2.ERASE_BLK_EN;

	default:
		return (s->bits[10] & 0x40) > 0;

	}
}


typedef union _ocr_reg_t {
	BYTE bits[4];
	struct _fields {
		// bits[0]
        uint8_t s18a:1;
        uint8_t _rsvd2:2;
        uint8_t co2tb:1;
        uint8_t _rsvd1:1;
        uint8_t uhs2_cs:1;
        uint8_t ccs:1;
        uint8_t ready:1;

        // bits[1]
        uint8_t vcc28_29:1;
        uint8_t vcc29_30:1;
        uint8_t vcc30_31:1;
        uint8_t vcc31_32:1;
        uint8_t vcc32_33:1;
        uint8_t vcc33_34:1;
        uint8_t vcc34_35:1;
        uint8_t vcc35_36:1;

        // bits[2]
		uint8_t _rsvd4:6;
		uint8_t lv_rsvd:1;
		uint8_t _rsvd3:7;
		uint8_t vcc27_28:1;
	} fields;
} ocr_reg_t;



DSTATUS dio_card_status;
BYTE dio_card_type;
csd_reg_t dio_csd_reg;


void DIO_SPI_DebugR1(BYTE r1) {
	DIO_SPI_PRINTF_TAG("R1 Result = %02X ", r1);
	if(r1 == 0xFF) {
		DIO_SPI_PRINTF("-> INVALID\n");

	} else {
		DIO_SPI_PRINTF("-> VALID\n");
		if(r1 & RES_R1_FAILED) DIO_SPI_PRINTF_TAG("\tInvalid result\n");
		if(r1 & RES_R1_IN_IDLE_STATE) DIO_SPI_PRINTF_TAG("\tIN_IDLE_STATE\n");
		if(r1 & RES_R1_ERASE_RESET) DIO_SPI_PRINTF_TAG("\tERASE_RESET\n");
		if(r1 & RES_R1_ILLEGAL_COMMAND) DIO_SPI_PRINTF_TAG("\tILLEGAL_COMMAND\n");
		if(r1 & RES_R1_COM_CRC_ERROR) DIO_SPI_PRINTF_TAG("\tCOMMAND CRC ERROR\n");
		if(r1 & RES_R1_ERASE_SEQ_ERROR) DIO_SPI_PRINTF_TAG("\tERASE_SEQ_ERROR\n");
		if(r1 & RES_R1_ADDRESS_ERROR) DIO_SPI_PRINTF_TAG("\tADDRESS_ERROR\n");
		if(r1 & RES_R1_PARAMETER_ERROR) DIO_SPI_PRINTF_TAG("\tPARAMETER_ERROR\n");

	}
}


void DIO_SPI_DebugR2(BYTE r2) {
	DIO_SPI_PRINTF_TAG("R2 Result = %02X\n", r2);
	if(r2 & RES_R2_CARD_IS_LOCKED) DIO_SPI_PRINTF_TAG("\tCARD_IS_LOCKED\n");
	if(r2 & RES_R2_WP_ERASE_SKIP) DIO_SPI_PRINTF_TAG("\tWP_ERASE_SKIP\n");
	if(r2 & RES_R2_ERROR) DIO_SPI_PRINTF_TAG("\tERROR");
	if(r2 & RES_R2_CC_ERROR) DIO_SPI_PRINTF_TAG("\tCC_ERROR\n");
	if(r2 & RES_R2_CARD_ECC_FAILED) DIO_SPI_PRINTF_TAG("\tCARD_ECC_FAILED\n");
	if(r2 & RES_R2_WP_VIOLATION) DIO_SPI_PRINTF_TAG("\tWP_VIOLATION\n");
	if(r2 & RES_R2_ERASE_PARAM) DIO_SPI_PRINTF_TAG("\tERASE_PARAM\n");
	if(r2 & RES_R2_OUT_OF_RANGE) DIO_SPI_PRINTF_TAG("\tOUT_OF_RANGE\n");
}


const BYTE dio_crc7_poly = 0x89;


BYTE DIO_SPI_CardCRC7(BYTE* pcrc, BYTE b) {
	uint8_t crc = (*pcrc);

	crc ^= b;
	for(uint8_t i = 0; i < 8; i++) {
		if(crc & 0x80) crc ^= dio_crc7_poly;
		crc <<= 1;
	}

	(*pcrc) = crc;

	return crc;
}


WORD DIO_SPI_CardCRC16(WORD* pcrc, BYTE b) {
	WORD crc = (*pcrc);

	crc = (uint8_t)(crc >> 8)|(crc << 8);
	crc ^= b;
	crc ^= (uint8_t)(crc & 0xFF) >> 4;
	crc ^= (crc << 8) << 4;
	crc ^=((crc & 0xff) << 4) << 1;

	(*pcrc) = crc;

	return crc;
}


_DIO_SPI_Delay_cb_t DIO_SPI_Delay_cb = NULL;


void DIO_SPI_Delay(uint32_t waitMs) {
	if(DIO_SPI_Delay_cb) DIO_SPI_Delay_cb(waitMs);
	else HAL_Delay(waitMs);
}


void DIO_SPI_CardDisable() {
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}


void DIO_SPI_CardEnable() {
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}


BYTE DIO_SPI_Transceive(BYTE data) {
	BYTE out;
	HAL_SPI_TransmitReceive(&SD_HSPI, &data, &out, 1, 50);
	return out;
}


void DIO_SPI_ReceiveMulti(BYTE* buf, UINT len, WORD* pcrc) {
	for(UINT i = 0; i < len; i++) {
		buf[i] = DIO_SPI_Transceive(0xFF);
		DIO_SPI_CardCRC16(pcrc, buf[i]);
	}
}


#if _USE_WRITE
void DIO_SPI_TransmitMulti(const BYTE* buf, UINT len, WORD* pcrc) {
	for(UINT i = 0; i < len; i++) {
		DIO_SPI_Transceive(buf[i]);
		DIO_SPI_CardCRC16(pcrc, buf[i]);
	}
}
#endif /* _USE_WRITE */


/* card methods */
DRESULT DIO_SPI_WaitCardReadyTim(dio_timer_t* tm) {
	BYTE tmp;

	do {
		tmp = DIO_SPI_Transceive(0xFF);
	} while(tmp != 0xFF && Util_TimerPending(tm));

	return (tmp == 0xFF) ? RES_OK : RES_NOTRDY;
}


DRESULT DIO_SPI_WaitCardReady(UINT timeout) {
	dio_timer_t tm;
	Util_TimerStart(&tm, timeout);
	return DIO_SPI_WaitCardReadyTim(&tm);
}


DRESULT DIO_SPI_CardDeselect() {
	DIO_SPI_CardDisable();
	DIO_SPI_Transceive(0xFF);
	return RES_OK;
}


DRESULT DIO_SPI_CardSelect() {
	DIO_SPI_CardEnable();
	DIO_SPI_Transceive(0xFF);
	if(DIO_SPI_WaitCardReady(500) == RES_OK) return RES_OK;

	DIO_SPI_CardDeselect();
	return RES_NOTRDY;
}


DRESULT DIO_SPI_RecvData(BYTE* buf, UINT len) {
	dio_timer_t tm;
	Util_TimerStart(&tm, 1000);

	BYTE tmp;
	do {
		tmp = DIO_SPI_Transceive(0xFF);
	} while(tmp == 0xFF && Util_TimerPending(&tm));
	if(tmp != 0xFE) return RES_ERROR;

	WORD realCrc = 0;
	DIO_SPI_ReceiveMulti(buf, len, &realCrc);

	WORD crc = 0;
	crc |= DIO_SPI_Transceive(0xFF); crc <<= 8;
	crc |= DIO_SPI_Transceive(0xFF);

	if(crc != realCrc) return RES_ERROR;

	return RES_OK;
}


#if _USE_WRITE
DRESULT DIO_SPI_SendData(const BYTE* buf, BYTE token) {
	if(DIO_SPI_WaitCardReady(500) != RES_OK) return RES_NOTRDY;

	DIO_SPI_Transceive(token);
	if(token != 0xFD) {
		WORD crc = 0;
		DIO_SPI_TransmitMulti(buf, 512, &crc);
		DIO_SPI_Transceive((uint8_t)(crc>>8));
		DIO_SPI_Transceive((uint8_t)(crc));

		BYTE res = DIO_SPI_Transceive(0xFF);
		if((res & 0x1F) != 0x05) return RES_ERROR;
	}

	return RES_OK;
}
#endif /* _USE_WRITE */


BYTE DIO_SPI_SendCmd(BYTE cmd, DWORD arg, BYTE* rsp, UINT rsplen) {
	BYTE res;

	if(cmd & 0x80) {
		cmd &= 0x7F;
		res = DIO_SPI_SendCmd(CMD55, 0, NULL, 0);
		if(R1_IS_ERROR(res)) return res;
	}

	DIO_SPI_CardDisable();
	DIO_SPI_Transceive(0xFF);

	DIO_SPI_CardEnable();

	BYTE crc = 0;
	DIO_SPI_Transceive(0x40 | cmd); DIO_SPI_CardCRC7(&crc, 0x40 | cmd);
	DIO_SPI_Transceive((BYTE)(arg >> 24)); DIO_SPI_CardCRC7(&crc, (BYTE)(arg >> 24));
	DIO_SPI_Transceive((BYTE)(arg >> 16)); DIO_SPI_CardCRC7(&crc, (BYTE)(arg >> 16));
	DIO_SPI_Transceive((BYTE)(arg >> 8)); DIO_SPI_CardCRC7(&crc, (BYTE)(arg >> 8));
	DIO_SPI_Transceive((BYTE)(arg)); DIO_SPI_CardCRC7(&crc, (BYTE)(arg));

	crc = crc | 0x01;
	DIO_SPI_Transceive(crc);

	DIO_SPI_CMD_PRINTF_TAG("cmd %i arg %08lX crc %02X", cmd, arg, crc);

	BYTE n = 10;
	do {
		res = DIO_SPI_Transceive(0xFF);
		n--;
	} while((res == 0xFF) && n);

	DIO_SPI_CMD_PRINTF(" res %02X\n", res);

	if(!rsp || !rsplen) return res;

	do {
		(*rsp) = DIO_SPI_Transceive(0xFF);
		rsp ++; rsplen --;
	} while(rsplen);

	return res;
}


DRESULT  DIO_SPI_reread_csd() {
	BYTE r1s = DIO_SPI_SendCmd(CMD9, 0, NULL, 0);
	if(R1_IS_ERROR(r1s)) return RES_ERROR;

	DIO_SPI_RecvData((uint8_t*)&dio_csd_reg, 16);

	return RES_OK;
}


/* public interface */


DSTATUS DIO_SPI_initialize_MMC(dio_timer_t* tm) {
	return RES_NOTRDY; // not implemented yet
}


DSTATUS DIO_SPI_initialize_SDV2(dio_timer_t* tm) {
	DIO_SPI_PRINTF_TAG("card is SD v2\n");

	BYTE res;

	DWORD arg = 1UL << 30;
	while(Util_TimerPending(tm)) {
		res = DIO_SPI_SendCmd(ACMD41, arg, NULL, 0);
		if(!(res & RES_R1_IN_IDLE_STATE)) break;
	}
	if(!Util_TimerPending(tm)) {
		DIO_SPI_PRINTF_TAG("ACMD41 timeout\n");
		return RES_ERROR;
	}

	dio_card_type = CT_SD2;

	BYTE ocr[4];
	ocr_reg_t* ocrt = (ocr_reg_t*)ocr;
	while(Util_TimerPending(tm)) {
		res = DIO_SPI_SendCmd(CMD58, 0, ocr, 4);
		if(!R1_IS_ERROR(res)) {
			if(ocrt->fields.ready) {
				DIO_SPI_PRINTF_TAG("card is powered up\n");

			} else {
				DIO_SPI_PRINTF_TAG("card is NOT powered up\n");
				DIO_SPI_Delay(10);
				continue;
			}

			if(ocrt->fields.ccs) {
				dio_card_type |= CT_BLOCK;
				DIO_SPI_PRINTF_TAG("card is SDHC\n");

			} else {
				DIO_SPI_PRINTF_TAG("card is SDSC\n");

			}
			break;
		}
	}
	if(!Util_TimerPending(tm)) {
		DIO_SPI_PRINTF_TAG("CMD58 timeout\n");
		return RES_ERROR;
	}

	// read csd
	return DIO_SPI_reread_csd();
}


DSTATUS DIO_SPI_initialize(BYTE pdrv) {
	if(pdrv != 0) return STA_NOINIT;

	if(dio_card_status & STA_NODISK) return dio_card_status;

	// dummy clocks to synchronize
	DIO_SPI_CardDisable();
	for(uint8_t i = 10; i; i--) DIO_SPI_Transceive(0xFF);

	dio_timer_t tm;
	Util_TimerStart(&tm, 1000);

	BYTE res;

	res = DIO_SPI_CardSelect();
	if(res != RES_OK) {
		DIO_SPI_PRINTF_TAG("SELECT failure\n");
		goto fail;
	}

	while(Util_TimerPending(&tm)) {
		res = DIO_SPI_SendCmd(CMD0, 0, NULL, 0);
		if(res == RES_R1_IN_IDLE_STATE) break;
		DIO_SPI_Delay(10);
	}
	if(R1_IS_ERROR(res)) {
		DIO_SPI_PRINTF_TAG("CMD0 failed\n");
		DIO_SPI_DebugR1(res);

		goto fail;
	}


	// enable crc
	res = DIO_SPI_SendCmd(CMD59, 0x01, NULL, 0);
	if(R1_IS_ERROR(res)) {
		DIO_SPI_PRINTF_TAG("CMD59 error, no CRC\n");
		DIO_SPI_DebugR1(res);
	}

	BYTE r7s[4];

	while(Util_TimerPending(&tm)) {
		res = DIO_SPI_SendCmd(CMD8, 0x01AA, r7s, 4);
		if(res & RES_R1_ILLEGAL_COMMAND) { // sd v1 or mmc
			res = DIO_SPI_initialize_MMC(&tm);
			if(res == RES_OK) break;

		} else { // sd v2
			if(r7s[2] != 0x01) {
				DIO_SPI_PRINTF_TAG("CMD8 voltage unsupported, got %02X\n", r7s[2]);
			}
			if(r7s[3] == 0xAA) {
				res = DIO_SPI_initialize_SDV2(&tm);
				if(res == RES_OK) break;
			} else {
				DIO_SPI_PRINTF_TAG("CMD8 bad echo %02X != %02X\n", 0xAA, r7s[3]);
			}
		}
	}
	if(!Util_TimerPending(&tm)) { // total timeout reached
		DIO_SPI_PRINTF_TAG("SD Init timeout\n");

		goto fail;
	}

	DIO_SPI_CardDeselect();

	dio_card_status &= ~STA_NOINIT;
	return dio_card_status;

fail:
	dio_card_type = 0;
	dio_card_status = STA_NOINIT;
	DIO_SPI_CardDeselect();
	return dio_card_status;
}


DSTATUS DIO_SPI_status(BYTE pdrv) {
	if(pdrv) return STA_NOINIT;
	return dio_card_status;
}


DRESULT DIO_SPI_read(BYTE pdrv, BYTE* buf, DWORD sector, UINT count) {
	if(pdrv) return RES_PARERR;
	if(!count) return RES_PARERR;

	if(dio_card_status & STA_NOINIT) return RES_NOTRDY;

	if(!(dio_card_type & CT_BLOCK)) sector <<= 9; // byte-addressed card

	BYTE res;
	if(count == 1) { // single sector read
		res = DIO_SPI_SendCmd(CMD17, sector, NULL, 0);
		if(res == RES_OK) {
			res = DIO_SPI_RecvData(buf, 512);
			if(res == RES_OK) count = 0;
			else {
				DIO_SPI_PRINTF_TAG("SINGLE_READ RecvData error=%i\n", res);
			}

		} else {
			DIO_SPI_PRINTF_TAG("SINGLE_READ SendCmd error\n");
			DIO_SPI_DebugR1(res);

		}

	} else { // block read
		res = DIO_SPI_SendCmd(CMD18, sector, NULL, 0);
		if(res == RES_OK) {
			do {
				res = DIO_SPI_RecvData(buf, 512);
				if(res != RES_OK) {
					DIO_SPI_PRINTF_TAG("BLOCK_READ RecvData error=%i\n", res);
					break;
				}
				buf += 512; count--;

			} while(count);
			if(count) {
				DIO_SPI_PRINTF_TAG("BLOCK_READ partial count=%i\n", count);
			}

			res = DIO_SPI_SendCmd(CMD12, 0, NULL, 0);
			if(res != RES_OK) {
				DIO_SPI_PRINTF_TAG("BLOCK_READ StopTrans error=%i\n", res);
			}

		} else {
			DIO_SPI_PRINTF_TAG("BLOCK_READ SendCmd error\n");
			DIO_SPI_DebugR1(res);

		}

	}
	DIO_SPI_CardDeselect();

	DIO_SPI_PRINTF_TAG("READ sector=%lu result=%i count=%i\n", sector, res, count);

	return count ? RES_ERROR : RES_OK;
}


#if _USE_WRITE
DRESULT DIO_SPI_write(BYTE pdrv, const BYTE* buf, DWORD sector, UINT count) {
	if(pdrv) return RES_PARERR;
	if(!count) return RES_PARERR;
	if(dio_card_status & STA_NOINIT) return RES_NOTRDY;
	if(dio_card_status & STA_PROTECT) return RES_WRPRT;

	if(!(dio_card_type & CT_BLOCK)) sector <<= 9; // byte-addressed card

	BYTE res;
	if(count == 1) { // single sector write
		DIO_SPI_PRINTF_TAG("SINGLE_WRITE p=%li l=%u\n", sector, count)
		res = DIO_SPI_SendCmd(CMD24, sector, NULL, 0);
		if(res == RES_OK) {
			res = DIO_SPI_SendData(buf, 0xFE);
			if(res == RES_OK) {
				count = 0;

			} else {
				DIO_SPI_PRINTF_TAG("SINGLE_WRITE SendData error=%i\n", res);

			}

		} else {
			DIO_SPI_PRINTF_TAG("SINGLE_WRITE SendCmd error=%i\n", res);
			DIO_SPI_DebugR1(res);

		}
	} else { // multi-sector write
		DIO_SPI_PRINTF_TAG("BLOCK_WRITE p=%li l=%u\n", sector, count)
		if(dio_card_type & CT_SDC) {
			res = DIO_SPI_SendCmd(ACMD23, count, NULL, 0); // prepare sectors
			if(res) {
				DIO_SPI_PRINTF_TAG("BLOCK_WRITE prepare error=%i\n", res);
			}

		}

		res = DIO_SPI_SendCmd(CMD25, sector, NULL, 0);
		if(res == RES_OK) { // block write
			do {
				res = DIO_SPI_SendData(buf, 0xFC);
				if(res != RES_OK) {
					DIO_SPI_PRINTF_TAG("BLOCK_WRITE SendData error=%i\n", res);
					break;

				}
				buf += 512; count--;

			} while(count);
			if(count) {
				DIO_SPI_PRINTF_TAG("BLOCK_WRITE partial count=%i\n", count);
			}

			res = DIO_SPI_SendData(NULL, 0xFD);
			if(res != RES_OK) {
				DIO_SPI_PRINTF_TAG("BLOCK_WRITE StopTrans error=%i\n",res);
				count = 1; // something wrong happens

			}

		} else {
			DIO_SPI_PRINTF_TAG("BLOCK_WRITE SendCmd error=%i\n", res);
			DIO_SPI_DebugR1(res);

		}
	}

	res = DIO_SPI_WaitCardReady(1000);
	if(res != RES_OK) {
		DIO_SPI_PRINTF_TAG("WRITE WaitReady error=%i\n", res);
		count = 1;
	}

	uint8_t r1, r2;
	r1 = DIO_SPI_SendCmd(CMD13, 0, &r2, 1);
	DIO_SPI_DebugR1(r1);
	DIO_SPI_DebugR2(r2);

	DIO_SPI_CardDeselect();

	DIO_SPI_PRINTF_TAG("WRITE p=%lu result=%i count=%i\n", sector, res, count);

	return count ? RES_ERROR : RES_OK;
}
#endif /* _USE_WRITE */


#if _USE_IOCTL
DRESULT DIO_SPI_ioctl_SD_GET_SECTOR_COUNT(BYTE pdrv, DWORD* buf) {
	uint32_t blknr = (csd_c_size(&dio_csd_reg) + 1) * csd_c_size_mult(&dio_csd_reg);
	if(!blknr) return RES_ERROR;

	(*buf) = blknr;

	return RES_OK;
}


DRESULT DIO_SPI_ioctl_SD_GET_SECTOR_SIZE(BYTE pdrv, WORD* buf) {
	(*buf) = 512;

	return RES_OK;
}


DRESULT DIO_SPI_ioctl_SD_GET_BLOCK_SIZE(BYTE pdrv, DWORD* buf) {
	uint32_t blks =  csd_sector_size(&dio_csd_reg) * csd_write_bl_len(&dio_csd_reg);
	if(!blks) return RES_ERROR;

	(*buf) = blks;

	return RES_OK;

}


DRESULT DIO_SPI_ioctl_SD_TRIM(BYTE pdrv, DWORD startblk, DWORD endblk) {
	if(dio_card_status & STA_NOINIT) return RES_NOTRDY;

	if(!csd_erase_block_enabled(&dio_csd_reg)) return RES_PARERR;

	if(!(dio_card_type & CT_BLOCK)) {
		startblk <<= 9; endblk <<= 9;
	}

	BYTE r1s;

	r1s = DIO_SPI_SendCmd(CMD32, startblk, NULL, 0);
	if(R1_IS_ERROR(r1s)) return RES_ERROR;

	r1s = DIO_SPI_SendCmd(CMD33, endblk, NULL, 0);
	if(R1_IS_ERROR(r1s)) return RES_ERROR;

	r1s = DIO_SPI_SendCmd(CMD38, 0, NULL, 0);
	if(R1_IS_ERROR(r1s)) return RES_ERROR;

	return DIO_SPI_WaitCardReady(30000);
}


DRESULT DIO_SPI_ioctl(BYTE pdrv, BYTE cmd, void* buf) {
	DIO_SPI_PRINTF_TAG("IOCTL pdrv=%i cmd=%02X\n", pdrv, cmd);

	if(pdrv) return RES_PARERR;
	if(dio_card_status & STA_NOINIT) return RES_NOTRDY;

	DRESULT res = RES_ERROR;

	DWORD* dptr;
	WORD* wptr;

	switch(cmd) {
	case CTRL_SYNC:
		if(DIO_SPI_CardSelect() == RES_OK) res = RES_OK;
		break;

	case CTRL_TRIM:
		dptr = buf;
		res = DIO_SPI_ioctl_SD_TRIM(pdrv, dptr[0], dptr[1]);
		break;

	case GET_SECTOR_COUNT:
		dptr = buf;
		res = DIO_SPI_ioctl_SD_GET_SECTOR_COUNT(pdrv, dptr);
		break;

	case GET_SECTOR_SIZE:
		wptr = buf;
		res = DIO_SPI_ioctl_SD_GET_SECTOR_SIZE(pdrv, wptr);
		break;

	case GET_BLOCK_SIZE:
		dptr = buf;
		res = DIO_SPI_ioctl_SD_GET_BLOCK_SIZE(pdrv, buf);
		break;

	default:
		res = RES_PARERR;

	}

	DIO_SPI_CardDeselect();

	return res;
}
#endif /* _USE_IOCTL */



Diskio_drvTypeDef DIO_SPI_Driver =
{
  DIO_SPI_initialize,
  DIO_SPI_status,
  DIO_SPI_read,
#if  _USE_WRITE
  DIO_SPI_write,
#endif  /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
  DIO_SPI_ioctl,
#endif /* _USE_IOCTL == 1 */
};

