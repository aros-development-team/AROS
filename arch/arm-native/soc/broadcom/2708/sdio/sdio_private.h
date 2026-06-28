/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Private state for sdio.resource: an SDIO host bound to the BCM2835
    Arasan SDHCI controller (peripheral offset 0x300000). On the Raspberry
    Pi 3/3B/4 this controller is wired to the on-board Broadcom WiFi chip
    over a 4-bit SDIO bus; the SD card itself uses the separate SDHOST
    controller (see rom/devs/sdcard). The resource exposes a generic
    SDIO function-I/O API (CMD52/CMD53) for client drivers (e.g. bwfm).
*/

#ifndef SDIO_PRIVATE_H_
#define SDIO_PRIVATE_H_

#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <inttypes.h>

struct SDIOBase
{
    struct Node             sdio_Node;
    struct SignalSemaphore  sdio_Sem;       /* Serialises command issue */
    unsigned int            sdio_periiobase;
    unsigned int            sdio_iobase;    /* Arasan SDHCI MMIO base */
    unsigned int            sdio_ClockMax;  /* Controller base clock (Hz) */
    uint16_t                sdio_RCA;       /* Relative card address (CMD3) */
    uint32_t                sdio_OCR;       /* I/O OCR from CMD5 */
    uint8_t                 sdio_NumFunc;   /* I/O function count from CMD5 */
    int                     sdio_Present;   /* An SDIO device answered CMD5 */
    unsigned int            sdio_LastWrite; /* SYSTIMER stamp of last MMIO write */
    uint16_t                sdio_BlkSize[8];/* per-function block size (CMD53 block mode) */

    /* SDIO card-interrupt (DAT1) routing to a consumer task (the RX pump) */
    APTR                    sdio_IRQHandle; /* KrnAddIRQHandler handle */
    struct Task            *sdio_IRQTask;   /* task to Signal on the card int */
    uint32_t                sdio_IRQSig;    /* signal mask to send it */
};

#define ARM_PERIIOBASE SDIOBase->sdio_periiobase
#include <hardware/bcm2708.h>

/*
 * SDIO-specific command opcodes (not in hardware/mmc.h, which only covers
 * the memory-card command set).
 */
#define SDIO_CMD_IO_SEND_OP_COND        5       /* R4 */
#define SDIO_CMD_IO_RW_DIRECT           52      /* R5 - CMD52 */
#define SDIO_CMD_IO_RW_EXTENDED         53      /* R5 - CMD53 */

/* OCR (CMD5 response) fields */
#define SDIO_OCR_NUM_FUNC_SHIFT         28
#define SDIO_OCR_NUM_FUNC_MASK          (0x7 << SDIO_OCR_NUM_FUNC_SHIFT)
#define SDIO_OCR_MEM_PRESENT            (1 << 27)
#define SDIO_OCR_IOREADY                (1 << 31)
#define SDIO_OCR_VOLTAGE_MASK           0x00ffff00      /* 2.0-3.6V window */

/* CMD52/CMD53 argument layout (IO_RW_DIRECT / IO_RW_EXTENDED) */
#define SDIO_ARG_RW_WRITE               (1u << 31)
#define SDIO_ARG_FUNC_SHIFT             28
#define SDIO_ARG_RAW                    (1u << 27)      /* read-after-write */
#define SDIO_ARG_BLOCKMODE              (1u << 27)      /* CMD53: block mode */
#define SDIO_ARG_OPCODE_INCR            (1u << 26)      /* CMD53: address increments */
#define SDIO_ARG_ADDR_SHIFT             9
#define SDIO_ARG_ADDR_MASK              0x1ffff
#define SDIO_ARG_COUNT_MASK             0x1ff           /* CMD53 byte/block count */

/* R5 response (CMD52/53) flags byte (bits 15:8 of the 32-bit response).
 * Bits 5:4 are IO_CURRENT_STATE (card state: 0=DIS, 1=CMD, 2=TRN) - NOT an
 * error, so they must be excluded from the error check (a selected card
 * reports state 1 = bit4 set on every CMD52/CMD53). */
#define SDIO_R5_OUT_OF_RANGE            (1 << 0)
#define SDIO_R5_FUNCTION_NUMBER         (1 << 1)
#define SDIO_R5_ERROR                   (1 << 3)
#define SDIO_R5_ILLEGAL_COMMAND         (1 << 6)
#define SDIO_R5_COM_CRC_ERROR           (1 << 7)
#define SDIO_R5_ERRORS                  (SDIO_R5_COM_CRC_ERROR | SDIO_R5_ILLEGAL_COMMAND | \
                                         SDIO_R5_ERROR | SDIO_R5_FUNCTION_NUMBER | \
                                         SDIO_R5_OUT_OF_RANGE)

/* CCCR (function 0) register offsets */
#define SDIO_CCCR_CCCR_REV              0x00
#define SDIO_CCCR_SD_REV                0x01
#define SDIO_CCCR_IO_ENABLE             0x02
#define SDIO_CCCR_IO_READY              0x03
#define SDIO_CCCR_INT_ENABLE            0x04
#define  SDIO_CCCR_IEN_MASTER           0x01    /* IENM: master int enable */
#define  SDIO_CCCR_IEN_FUNC1            0x02    /* IEN1: function 1 int enable */
#define SDIO_CCCR_INT_PENDING           0x05
#define SDIO_CCCR_BUS_CONTROL           0x07
#define SDIO_CCCR_BUS_WIDTH_4BIT        0x02
#define SDIO_CCCR_BUS_WIDTH_MASK        0x03
#define SDIO_CCCR_CAPABILITY            0x08
#define SDIO_CCCR_HIGH_SPEED            0x13
#define  SDIO_CCCR_SHS                  0x01    /* supports high speed */
#define  SDIO_CCCR_EHS                  0x02    /* enable high speed */
#define SDIO_CCCR_FN0_BLOCK_SIZE0       0x10    /* FBR-style: F0 block size */
#define SDIO_CCCR_FN0_BLOCK_SIZE1       0x11

/* Per-function FBR base (function n at 0x100 * n), block-size at +0x10/0x11 */
#define SDIO_FBR_BASE(f)                (0x100 * (f))
#define SDIO_FBR_BLOCK_SIZE0            0x10
#define SDIO_FBR_BLOCK_SIZE1            0x11

#endif /* SDIO_PRIVATE_H_ */
