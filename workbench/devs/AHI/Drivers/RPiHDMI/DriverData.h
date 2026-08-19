/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi HDMI Audio AHI Sub-Driver Data Structures
*/

#ifndef AHI_Drivers_RPiHDMI_DriverData_h
#define AHI_Drivers_RPiHDMI_DriverData_h

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include "DriverBase.h"

/*
 * BCM DMA Control Block - 32-byte aligned
 */
struct DMAControlBlock {
    ULONG ti;          /* Transfer Information */
    ULONG source_ad;   /* Source physical bus address */
    ULONG dest_ad;     /* Destination physical bus address */
    ULONG txfr_len;    /* Transfer length in bytes */
    ULONG stride;      /* 2D stride (0 for linear) */
    ULONG nextconbk;   /* Next CB address (0 = stop) */
    ULONG reserved[2]; /* 32-byte alignment padding */
};

/*
 * Driver library base
 */
struct RPiHDMIBase {
    struct DriverBase driverbase;
    struct DosLibrary *dosbase;
    IPTR periiobase;
    IPTR hdmi_base;
};

#define DRIVERBASE_SIZEOF (sizeof(struct RPiHDMIBase))
#define DOSBase (*(struct DosLibrary **) &RPiHDMIBase->dosbase)

/*
 * Per-audio-context driver data (stored in ahiac_DriverData)
 */
struct RPiHDMIData {
    struct DriverData driverdata;
    UBYTE flags;
    UBYTE pad1;
    BYTE mastersignal;
    BYTE slavesignal;
    struct Process *mastertask;
    struct Process *slavetask;
    struct RPiHDMIBase *ahisubbase;

    /* Hardware MMIO bases & DMA channel */
    IPTR periiobase;
    IPTR hdmi_base;
    ULONG dma_channel;

    /* DMA control blocks */
    struct DMAControlBlock *cb_base;
    struct DMAControlBlock *cb[2];

    /* Sample buffers */
    APTR mixbuffer;
    ULONG *dmabuf[2];
    ULONG dmabuf_size;
    ULONG dmabuf_samples;

    /* IRQ & Config */
    APTR irq_handle;
    ULONG samplerate;
    ULONG channels;
};

#endif /* AHI_Drivers_RPiHDMI_DriverData_h */
