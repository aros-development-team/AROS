#ifndef _SDCARDSDHOST_INTERN_H
#define _SDCARDSDHOST_INTERN_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BCM2835 SDHOST controller driver internals.
*/

#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <utility/utility.h>
#include <exec/io.h>
#include <exec/errors.h>

#include <devices/timer.h>

extern IPTR __arm_periiobase;
#define ARM_PERIIOBASE __arm_periiobase
#include <hardware/bcm2708.h>
#include <hardware/bcm2708_dma.h>
#include <hardware/sdhost.h>

#include "sdcard_base.h"

#define FNAME_SDHOST(x)                 SDHOST__Device__ ## x
#define FNAME_SDHOSTBUS(x)             SDHOST__SDBus__ ## x

/* Private data stored in sdcb_Private (cast to struct sdhost_private *) */
struct sdhost_private {
    ULONG   max_clk;            /* Core clock rate from VideoCore */
    ULONG   cdiv;               /* Cached SDCDIV value */
    ULONG   hcfg;               /* Cached SDHCFG value */

    /* DMA state */
    LONG    dma_channel;        /* Channel allocated from dma.resource */
    APTR    dma_cb_raw;         /* Unaligned alloc for CB */
    struct BCM2708DMACB *dma_cb; /* 32-byte aligned control block */
    ULONG   dma_cb_raw_size;    /* Allocation size */

    /* Bounce buffer used for DMA reads into possibly mis-aligned caller buffers. */
    APTR    dma_bounce_raw;     /* Unaligned bounce buffer alloc */
    APTR    dma_bounce;         /* 32-byte aligned bounce buffer */
    ULONG   dma_bounce_size;    /* Bounce buffer size */

    /* Transient transfer bookkeeping (set in SendCmd). */
    BOOL    xfer_active;
    BOOL    xfer_is_dma;
    BOOL    dma_active;
    ULONG   dma_data_addr;
    ULONG   dma_data_len;
    ULONG   dma_xfer_len;
};

/* Accessor for bus private data */
#define SDHOST_PRIV(bus)  ((struct sdhost_private *)(bus)->sdcb_Private)

/* SDHOST register read/write — direct MMIO */
static inline ULONG sdhost_read(struct sdcard_Bus *bus, ULONG reg)
{
    return AROS_LE2LONG(*(volatile ULONG *)((ULONG)bus->sdcb_IOBase + reg));
}

static inline void sdhost_write(struct sdcard_Bus *bus, ULONG reg, ULONG val)
{
    *(volatile ULONG *)((ULONG)bus->sdcb_IOBase + reg) = AROS_LONG2LE(val);
}

/* Function declarations */
void  FNAME_SDHOSTBUS(SoftReset)(UBYTE mask, struct sdcard_Bus *bus);
void  FNAME_SDHOSTBUS(SetClock)(ULONG speed, struct sdcard_Bus *bus);
void  FNAME_SDHOSTBUS(SetPowerLevel)(ULONG supportedlvls, BOOL lowest, struct sdcard_Bus *bus);
ULONG FNAME_SDHOSTBUS(SendCmd)(struct TagItem *CmdTags, struct sdcard_Bus *bus);
ULONG FNAME_SDHOSTBUS(WaitCmd)(ULONG mask, ULONG timeout, struct sdcard_Bus *bus);
ULONG FNAME_SDHOSTBUS(FinishCmd)(struct TagItem *CmdTags, struct sdcard_Bus *bus);
ULONG FNAME_SDHOSTBUS(FinishData)(struct TagItem *DataTags, struct sdcard_Bus *bus);
void  FNAME_SDHOSTBUS(BusIRQ)(struct sdcard_Bus *bus, void *unused);
void  FNAME_SDHOSTBUS(SetBusWidth)(UBYTE width, struct sdcard_Bus *bus);
void  FNAME_SDHOST(BusInit)(struct sdcard_Bus *bus);
void  FNAME_SDHOST(BusPostIRQInit)(struct sdcard_Bus *bus);

#endif /* _SDCARDSDHOST_INTERN_H */
