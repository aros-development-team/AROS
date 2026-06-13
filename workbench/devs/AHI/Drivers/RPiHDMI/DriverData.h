#ifndef AHI_Drivers_RPiHDMI_DriverData_h
#define AHI_Drivers_RPiHDMI_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include "DriverBase.h"

/* Shared BCM2835 DMA control block layout (32-byte aligned). */
#include <hardware/bcm2708_dma.h>

/*
 * Driver library base
 */
struct RPiHDMIBase {
    struct DriverBase driverbase;
    struct DosLibrary *dosbase;
    ULONG periiobase;
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

    /* Hardware state */
    ULONG periiobase;
    LONG dma_channel;     /* Allocated from dma.resource, -1 = none */

    /* DMA control blocks (32-byte aligned) */
    struct BCM2708DMACB *cb_base; /* Allocated block (for free) */
    struct BCM2708DMACB *cb[2];   /* Aligned pointers to CB A and CB B */

    /* Audio buffers */
    APTR mixbuffer;       /* AHI mix buffer (signed 16-bit) */
    ULONG *dmabuf[2];     /* SPDIF DMA buffers (32-bit subframes) */
    ULONG dmabuf_size;    /* Size of each DMA buffer in bytes */
    ULONG dmabuf_samples; /* Number of sample frames per buffer */

    /* IRQ */
    APTR irq_handle;

    /* Configuration */
    ULONG samplerate;

    /* IEC958 state — separate channel status for L and R per IEC 60958-3 */
    UBYTE channel_status_l[24]; /* Left channel status block (192 bits) */
    UBYTE channel_status_r[24]; /* Right channel status block (192 bits) */
    ULONG frame_counter;        /* Current frame within IEC958 block (0-191) */
};

#endif /* AHI_Drivers_RPiHDMI_DriverData_h */
