/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MMC_H
#define _MMC_H

#define MMC_STATUS_MASK                 ~(0x0206BF7F)
#define MMC_STATUS_RDY_FOR_DATA         (1 << 8)
#define MMC_STATUS_ERROR                (1 << 19)

#define MMC_STATUS_STATE_MASK           (0xF << 9)
#define MMC_STATE_PRG                   (7 << 9)

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

#endif /* _MMC_H */
