#ifndef HIDD_SCSI_H
#define HIDD_SCSI_H

/*
    Copyright © 2013-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SCSI bus driver HIDD definitions
    Lang: english
*/

#define CLID_Hidd_SCSI           "hidd.scsi"
#define CLID_Hidd_SCSIBus        "hidd.scsi.bus"

struct SCSI_BusInterface
{
    VOID  (*scsi_out    )(void *obj, UBYTE val, UWORD offset);
    UBYTE (*scsi_in     )(void *obj, UWORD offset);
    VOID  (*scsi_out_alt)(void *obj, UBYTE val, UWORD offset);
    UBYTE (*scsi_in_alt )(void *obj, UWORD offset);
};

struct SCSI_PIOInterface
{
    VOID  (*scsi_outsw  )(void *obj, APTR address, ULONG count);
    VOID  (*scsi_insw   )(void *obj, APTR address, ULONG count);
    VOID  (*scsi_outsl  )(void *obj, APTR address, ULONG count);
    VOID  (*scsi_insl   )(void *obj, APTR address, ULONG count);
};

struct SCSI_DMAInterface
{
    BOOL  (*dma_Prepare)(void *obj, APTR buffer, IPTR size, BOOL read);
    VOID  (*dma_Start  )(void *obj);
    VOID  (*dma_End    )(void *obj, APTR buffer, IPTR size, BOOL read);
    ULONG (*dma_Result )(void *obj);
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

   AB_XFER_48BIT = 27, /* LBA48         */
   AB_XFER_RWMULTI,    /* Multisector   */
   AB_XFER_PACKET,     /* ATAPI         */
   AB_XFER_LBA,        /* LBA28         */
   AB_XFER_PIO32       /* 32-bit PIO    */
} scsi_XferMode;

#define AF_XFER_PIO(x)  (1<<(AB_XFER_PIO0+(x)))
#define AF_XFER_MDMA(x) (1<<(AB_XFER_MDMA0+(x)))
#define AF_XFER_UDMA(x) (1<<(AB_XFER_UDMA0+(x)))
#define AF_XFER_48BIT   (1<<(AB_XFER_48BIT))
#define AF_XFER_RWMULTI (1<<(AB_XFER_RWMULTI))
#define AF_XFER_PACKET  (1<<(AB_XFER_PACKET))
#define AF_XFER_LBA     (1<<(AB_XFER_LBA))
#define AF_XFER_PIO32   (1<<(AB_XFER_PIO32))

//#include <interface/Hidd_SCSI.h>
#include <interface/Hidd_SCSIBus.h>
#include <interface/Hidd_SCSIUnit.h>

#endif
