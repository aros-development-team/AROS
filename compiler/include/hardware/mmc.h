/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MMC_H
#define _MMC_H

#define MMC_STATUS_MASK                 ~(0x0206BF7F)
#define MMC_STATUS_APP_CMD              (1 << 5)
#define MMC_STATUS_SWITCH_ERR           (1 << 7)
#define MMC_STATUS_RDY_FOR_DATA         (1 << 8)
#define MMC_STATUS_STATE_MASK           (0xF << 9)
#define MMC_STATUS_STATE_IDLE           (0 << 9)
#define MMC_STATUS_STATE_READY          (1 << 9)
#define MMC_STATUS_STATE_IDENT          (2 << 9)
#define MMC_STATUS_STATE_STBY           (3 << 9)
#define MMC_STATUS_STATE_TRAN           (4 << 9)
#define MMC_STATUS_STATE_DATA           (5 << 9)
#define MMC_STATUS_STATE_RCV            (6 << 9)
#define MMC_STATUS_STATE_PRG            (7 << 9)
#define MMC_STATUS_STATE_DIS            (8 << 9)
#define MMC_STATUS_ERASE_RESET          (1 << 13)
#define MMC_STATUS_WP_ERASE_SKIP        (1 << 15)
#define MMC_STATUS_CIDCSD_OVERWRITE     (1 << 16)
#define MMC_STATUS_OVERRUN              (1 << 17)
#define MMC_STATUS_UNDERRUN             (1 << 18)
#define MMC_STATUS_ERROR                (1 << 19)
#define MMC_STATUS_CC_ERROR             (1 << 20)
#define MMC_STATUS_CARD_ECC_FAILED      (1 << 21)
#define MMC_STATUS_ILLEGAL_COMMAND      (1 << 22)
#define MMC_STATUS_COM_CRC_ERROR        (1 << 23)
#define MMC_STATUS_UN_LOCK_FAILED       (1 << 24)
#define MMC_STATUS_CARD_IS_LOCKED       (1 << 25)
#define MMC_STATUS_WP_VIOLATION         (1 << 26)
#define MMC_STATUS_ERASE_PARAM          (1 << 27)
#define MMC_STATUS_ERASE_SEQ_ERROR      (1 << 28)
#define MMC_STATUS_BLOCK_LEN_ERROR      (1 << 29)
#define MMC_STATUS_ADDRESS_MISALIGN     (1 << 30)
#define MMC_STATUS_ADDR_OUT_OR_RANGE    (1 << 31)

/* MMC Command Responses */

#define MMC_RSP_PRESENT                 (1 << 0)
#define MMC_RSP_136	                (1 << 1)
#define MMC_RSP_CRC	                (1 << 2)
#define MMC_RSP_BUSY	                (1 << 3)
#define MMC_RSP_OPCODE	                (1 << 4)

#define MMC_RSP_NONE	                (0)
#define MMC_RSP_R1	                (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b	                (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2	                (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_136)
#define MMC_RSP_R3	                (MMC_RSP_PRESENT)
#define MMC_RSP_R4	                (MMC_RSP_PRESENT)
#define MMC_RSP_R5	                (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6	                (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	                (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

/* MMC Data Command Attributes */

#define MMC_DATA_WRITE                  (1 << 8)
#define MMC_DATA_READ                   (1 << 9)
#define MMC_DATA_STREAM                 (1 << 10)

/* MMC Commands */

#define MMC_CMD_GO_IDLE_STATE           0
#define MMC_CMD_SEND_OP_COND            1
#define MMC_CMD_ALL_SEND_CID            2
#define MMC_CMD_SET_RELATIVE_ADDR       3
#define MMC_CMD_SET_DSR                 4
#define MMC_CMD_SWITCH                  6
#define MMC_CMD_SELECT_CARD             7
#define MMC_CMD_SEND_EXT_CSD            8
#define MMC_CMD_SEND_CSD                9
#define MMC_CMD_SEND_CID                10
#define MMC_CMD_STOP_TRANSMISSION       12
#define MMC_CMD_SEND_STATUS             13
#define MMC_CMD_SET_BLOCKLEN            16
#define MMC_CMD_READ_SINGLE_BLOCK       17
#define MMC_CMD_READ_MULTIPLE_BLOCK     18
#define MMC_CMD_WRITE_SINGLE_BLOCK      24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK    25
#define MMC_CMD_ERASE_GROUP_START       35
#define MMC_CMD_ERASE_GROUP_END         36
#define MMC_CMD_ERASE                   38
#define MMC_CMD_APP_CMD                 55
#define MMC_CMD_SPI_READ_OCR            58
#define MMC_CMD_SPI_CRC_ON_OFF          59

/* MMC Operating Voltages */

#define MMC_VDD_165_195                 0x00000080
#define MMC_VDD_200_210                 0x00000100
#define MMC_VDD_210_220                 0x00000200
#define MMC_VDD_220_230                 0x00000400
#define MMC_VDD_230_240                 0x00000800
#define MMC_VDD_240_250                 0x00001000
#define MMC_VDD_250_260                 0x00002000
#define MMC_VDD_260_270                 0x00004000
#define MMC_VDD_270_280                 0x00008000
#define MMC_VDD_280_290                 0x00010000
#define MMC_VDD_290_300                 0x00020000
#define MMC_VDD_300_310                 0x00040000
#define MMC_VDD_310_320                 0x00080000
#define MMC_VDD_320_330                 0x00100000
#define MMC_VDD_330_340                 0x00200000
#define MMC_VDD_340_350                 0x00400000
#define MMC_VDD_350_360                 0x00800000

/* Ext_CSD */
#define EXT_CSD_HS_TIMING               185
#define EXT_CSD_CMD_SET_NORMAL          (1<<0)

#define MMC_HS_TIMING                   0x00000100
#define MMC_HS_52MHZ                    0x2

#endif /* _MMC_H */
