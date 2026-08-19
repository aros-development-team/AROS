/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    AHI Driver for Raspberry Pi I2S Audio DACs
*/

#ifndef AHI_Drivers_RPiI2S_DriverData_h
#define AHI_Drivers_RPiI2S_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include "DriverBase.h"

struct DMAControlBlock {
    ULONG ti, source_ad, dest_ad, txfr_len, stride, nextconbk, reserved[2];
};

struct RPiI2SBase {
    struct DriverBase driverbase;
    struct DosLibrary *dosbase;
    IPTR periiobase;
};
#define DRIVERBASE_SIZEOF (sizeof(struct RPiI2SBase))
#define DOSBase (*(struct DosLibrary **) &RPiI2SBase->dosbase)

struct RPiI2SData {
    struct DriverData driverdata;
    UBYTE flags, pad1;
    BYTE mastersignal, slavesignal;
    struct Process *mastertask, *slavetask;
    struct RPiI2SBase *ahisubbase;
    IPTR periiobase;
    IPTR i2s_base;          /* PCM/I2S register base */
    ULONG dma_channel;
    struct DMAControlBlock *cb_base, *cb[2];
    APTR mixbuffer;
    ULONG *dmabuf[2];
    ULONG dmabuf_size, dmabuf_samples;
    APTR irq_handle;
    ULONG samplerate;
};
#endif
