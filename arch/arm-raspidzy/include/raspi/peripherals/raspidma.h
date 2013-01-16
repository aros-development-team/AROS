/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RASPI_DMA_H
#define RASPI_DMA_H

#include <exec/types.h>
#include <stdint.h>

#define RASPIDMA0CH_PHYSBASE (IPTR)0x20007000
#define RASPIDMA1CH_PHYSBASE (IPTR)0x20007100
#define RASPIDMA2CH_PHYSBASE (IPTR)0x20007200
#define RASPIDMA3CH_PHYSBASE (IPTR)0x20007300
#define RASPIDMA4CH_PHYSBASE (IPTR)0x20007400
#define RASPIDMA5CH_PHYSBASE (IPTR)0x20007500
#define RASPIDMA6CH_PHYSBASE (IPTR)0x20007600
#define RASPIDMA7CH_PHYSBASE (IPTR)0x20007700
#define RASPIDMA8CH_PHYSBASE (IPTR)0x20007800
#define RASPIDMA9CH_PHYSBASE (IPTR)0x20007900
#define RASPIDMAACH_PHYSBASE (IPTR)0x20007A00
#define RASPIDMABCH_PHYSBASE (IPTR)0x20007B00
#define RASPIDMACCH_PHYSBASE (IPTR)0x20007C00
#define RASPIDMADCH_PHYSBASE (IPTR)0x20007D00
#define RASPIDMAECH_PHYSBASE (IPTR)0x20007E00
#define RASPIDMAFCH_PHYSBASE (IPTR)0x20E05000

#define RASPIDMASTATUS_PHYSBASE (IPTR)0x20007FE0

struct raspidmachannel {
    uint32_t cs;        // DMA Channel Control and Status
    uint32_t conblk_ad  // DMA Channel Control Block Address
    uint32_t ti         // DMA Channel CB Word 0 (Transfer Information)
    uint32_t source_ad  // DMA Channel CB Word 1 (Source Address)
    uint32_t dest_ad    // DMA Channel CB Word 2 (Destination Address)
    uint32_t txfr_len   // DMA Channel CB Word 3 (Transfer Length)
    uint32_t stride     // DMA Channel CB Word 4 (2D Stride)
    uint32_t nextconbk  // DMA Channel CB Word 5 (Next CB Address)
    uint32_t debug      // DMA Channel Debug
}__attribute__((packed));;

struct raspidmastatus {
    uint32_t int_status;    // Interrupt status of each DMA channel
    uint32_t enable;        // Global enable bits for each DMA channel
}__attribute__((packed));;

#endif /* RASPI_DMA */
