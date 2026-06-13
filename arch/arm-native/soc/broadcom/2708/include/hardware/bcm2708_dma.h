/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BCM2835/BCM2708 DMA controller — shared definitions for users of
    dma.resource. Register and TI/CS bit definitions live in bcm2708.h.
*/

#ifndef HARDWARE_BCM2708_DMA_H
#define HARDWARE_BCM2708_DMA_H

#include <exec/types.h>

/*
 * Channel allocation flags for DMAAllocChannel() (dma.resource).
 *
 * BCM283x has full-featured engines (channels 0-6: 2D/TDMODE, wide
 * bursts) and "lite" engines (channels 7-14: 32-bit transfers only, no
 * TDMODE, lower throughput). The VideoCore firmware reserves several, so
 * the resource only hands out a subset (full: 2, 4, 5; lite: 8-12). Lite
 * channels are handed out first unless DMACHF_TDMODE is requested,
 * keeping the scarce full engines available for users that need 2D
 * stride mode.
 */
#define DMACHF_TDMODE               (1 << 0)    /* Needs a full engine (2D stride mode) */
#define DMACHF_IRQ                  (1 << 1)    /* Resource owns the channel IRQ for
                                                 * DMAWaitChannel(). Leave unset when the
                                                 * driver installs its own handler (AHI). */

/* DMA control block — hardware-defined layout, must be 32-byte aligned.
 * All fields are little-endian; callers convert with AROS_LONG2LE. */
struct BCM2708DMACB
{
    ULONG   ti;             /* Transfer information */
    ULONG   source_ad;      /* Source bus address */
    ULONG   dest_ad;        /* Destination bus address */
    ULONG   txfr_len;       /* Transfer length (2D mode: YLENGTH<<16 | XLENGTH) */
    ULONG   stride;         /* 2D mode stride (signed 16-bit pair) */
    ULONG   nextconbk;      /* Next control block bus address, 0 = stop */
    ULONG   reserved[2];
};

/* System RAM must be presented to the DMA engine through the uncached
 * VideoCore bus alias. */
#define BCM2708_DMA_BUS_ADDR(x)     (0xC0000000 | (ULONG)(x))

#endif /* HARDWARE_BCM2708_DMA_H */
