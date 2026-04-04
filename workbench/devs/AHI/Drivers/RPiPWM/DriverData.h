#ifndef AHI_Drivers_RPiPWM_DriverData_h
#define AHI_Drivers_RPiPWM_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include "DriverBase.h"

/*
 * BCM2835 DMA Control Block - must be 32-byte aligned
 */
struct DMAControlBlock {
    ULONG ti;          /* Transfer Information */
    ULONG source_ad;   /* Source address (bus address) */
    ULONG dest_ad;     /* Destination address (bus address) */
    ULONG txfr_len;    /* Transfer length in bytes */
    ULONG stride;      /* 2D stride */
    ULONG nextconbk;   /* Next CB address (bus address), 0 = stop */
    ULONG reserved[2]; /* Padding to 32 bytes */
};

/*
 * Driver library base
 */
struct RPiPWMBase {
    struct DriverBase driverbase;
    struct DosLibrary *dosbase;
    ULONG periiobase;
};

#define DRIVERBASE_SIZEOF (sizeof(struct RPiPWMBase))

#define DOSBase (*(struct DosLibrary **) &RPiPWMBase->dosbase)

/*
 * Per-audio-context driver data (stored in ahiac_DriverData)
 */
struct RPiPWMData {
    struct DriverData driverdata;
    UBYTE flags;
    UBYTE pad1;
    BYTE mastersignal;
    BYTE slavesignal;
    struct Process *mastertask;
    struct Process *slavetask;
    struct RPiPWMBase *ahisubbase;

    /* Hardware state */
    ULONG periiobase;
    ULONG dma_channel;

    /* DMA control blocks (32-byte aligned) */
    struct DMAControlBlock *cb_base; /* Allocated block (for free) */
    struct DMAControlBlock *cb[2];   /* Aligned pointers to CB A and CB B */

    /* Audio buffers */
    APTR mixbuffer;       /* AHI mix buffer (signed 16-bit) */
    ULONG *dmabuf[2];     /* PWM DMA buffers (unsigned 32-bit) */
    ULONG dmabuf_size;    /* Size of each DMA buffer in bytes */
    ULONG dmabuf_samples; /* Number of sample frames per buffer */

    /* IRQ */
    APTR irq_handle;

    /* Configuration */
    ULONG samplerate;
    ULONG pwm_range;
};

#endif /* AHI_Drivers_RPiPWM_DriverData_h */
