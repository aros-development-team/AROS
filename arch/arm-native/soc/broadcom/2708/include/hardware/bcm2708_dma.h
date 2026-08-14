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

/*
 * The uncached VideoCore bus alias covers the first gigabyte only. Above that
 * the cast drops the high bits and the engine reads elsewhere, so check first.
 */
#define BCM2708_DMA_MAX_ADDR        0x40000000UL
#define BCM2708_DMA_ADDRESSABLE(x)  (((IPTR)(x)) < BCM2708_DMA_MAX_ADDR)
#define BCM2708_DMA_BUS_ADDR(x)     (0xC0000000 | (ULONG)(IPTR)(x))

/*
 * CS for a channel carrying a real-time stream: active, plus the AXI
 * priorities it needs to win arbitration while its FIFO runs dry.
 *
 * Acknowledge with CS_ACK, not a bare ACTIVE|INT|END - the priority fields
 * share the register, so a bare write drops the channel to the bottom of the
 * bus for the rest of the session. Nor a read-modify-write: CS carries status
 * bits (PAUSED, DREQ_STOPS_DMA) that must not be written back.
 *
 * Prefixed because the AHI drivers keep their own DMA_CS_* for single bits.
 */
#define BCM2708_DMA_CS_PRIORITY         8
#define BCM2708_DMA_CS_PANIC_PRIORITY   15

#define BCM2708_DMA_CS_RUN  ((1UL << 28) /* wait for outstanding writes */ | \
                             ((ULONG)BCM2708_DMA_CS_PANIC_PRIORITY << 20)  | \
                             ((ULONG)BCM2708_DMA_CS_PRIORITY << 16)        | \
                             (1UL << 0)  /* active */)

/* ... and with the write-1-to-clear completion flags added. */
#define BCM2708_DMA_CS_ACK  (BCM2708_DMA_CS_RUN | (1UL << 2) /* int */ | \
                                                  (1UL << 1) /* end */)

/*
 * Where a channel's completion arrives. Channel n is GPU IRQ 16+n, which the
 * BCM2711 presents through the GIC 96 higher - but not one line per channel:
 * 7 and 8 share, as do 9 and 10, so arithmetic is wrong from channel 8 on and
 * a handler there waits on a line the engine never raises.
 *
 * Self-contained (no ARM_PERIIOBASE) so the AHI drivers, which learn their
 * peripheral base at run time, can use it too. Sharing a line is safe as long
 * as a handler checks its own channel's CS.INT.
 */
#define BCM2708_DMA_IRQ_BASE        16          /* GPU IRQ of channel 0 */
#define BCM2708_DMA_PERIIOBASE_2711 0xFE000000
#define BCM2708_DMA_GPUIRQ_OFFSET   96

static inline unsigned int BCM2708_DMA_IRQ(IPTR periiobase, unsigned int channel)
{
    /* The SPI each channel raises on the BCM2711, in channel order. */
    static const UBYTE spi[] = { 80, 81, 82, 83, 84, 85, 86, 87, 87, 88, 88 };

    if (periiobase != BCM2708_DMA_PERIIOBASE_2711)
        return BCM2708_DMA_IRQ_BASE + channel;

    if (channel < sizeof(spi) / sizeof(spi[0]))
        return 32 + spi[channel];   /* the GIC presents SPI n as INTID 32+n */

    /* Not a channel this SoC hands out; keep the arithmetic answer. */
    return BCM2708_DMA_IRQ_BASE + channel + BCM2708_DMA_GPUIRQ_OFFSET;
}

#endif /* HARDWARE_BCM2708_DMA_H */
