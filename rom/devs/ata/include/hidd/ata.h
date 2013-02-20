#ifndef HIDD_ATA_H
#define HIDD_ATA_H

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ATA bus driver HIDD definitions
    Lang: english
*/

#define CLID_HW_ATA      "hw.ata"
#define CLID_Hidd_ATABus "hidd.ata.bus"

struct ATA_PIOInterface
{
    VOID  (*ata_out    )(void *obj, UBYTE val, UWORD offset);
    UBYTE (*ata_in     )(void *obj, UWORD offset);
    VOID  (*ata_out_alt)(void *obj, UBYTE val, UWORD offset);
    UBYTE (*ata_in_alt )(void *obj, UWORD offset);
    VOID  (*ata_outsw  )(void *obj, APTR address, ULONG count);
    UBYTE (*ata_insw   )(void *obj, APTR address, ULONG count);
    VOID  (*ata_ackInt )(void *obj);                            /* Optional */
};

struct ATA_PIO32Interface
{
    VOID  (*ata_outsl)(void *obj, APTR address, ULONG count);
    UBYTE (*ata_insl )(void *obj, APTR address, ULONG count);
};

struct ATA_DMAInterface
{
    BOOL  (*dma_Setup    )(void *obj, APTR buffer, IPTR size, BOOL read);
    VOID  (*dma_Cleanup  )(void *obj, APTR buffer, IPTR size, BOOL read);
    VOID  (*dma_Start    )(void *obj);
    VOID  (*dma_Stop     )(void *obj);
    BOOL  (*dma_IntStatus)(void *obj);
    ULONG (*dma_Result   )(void *obj);
};

typedef enum
{
   AB_XFER_PIO0 = 0,
   AB_XFER_PIO1,
   AB_XFER_PIO2,
   AB_XFER_PIO3,
   AB_XFER_PIO4,

   AB_XFER_MDMA0,
   AB_XFER_MDMA1,
   AB_XFER_MDMA2,

   AB_XFER_UDMA0,
   AB_XFER_UDMA1,
   AB_XFER_UDMA2,
   AB_XFER_UDMA3,
   AB_XFER_UDMA4,
   AB_XFER_UDMA5,
   AB_XFER_UDMA6,

   AB_XFER_48BIT,
   AB_XFER_RWMULTI,
   AB_XFER_PACKET,
   AB_XFER_LBA,
   AB_XFER_DMA,

} ata_XferMode;



#include <interface/Hidd_ATABus.h>

#endif
