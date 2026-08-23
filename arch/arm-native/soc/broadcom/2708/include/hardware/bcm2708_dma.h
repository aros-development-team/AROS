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
#define DMACHF_DMA4                 (1 << 2)    /* Wants a BCM2711 DMA4 engine. These are a
                                                 * different programming model, not just a
                                                 * faster channel - see BCM2711DMA4CB below -
                                                 * so they are never handed out unasked, and
                                                 * the request fails on other SoCs. DMA4 does
                                                 * 2D as well, so TDMODE need not be set. */

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
 * BCM2711 DMA4 control block - hardware-defined layout, must be 32-byte
 * aligned (the CB address is stored shifted right by 5).
 *
 * Not a superset of BCM2708DMACB: the two address words carry the low 32 bits
 * only, with the top 8 bits packed into the matching info word next to that
 * side's burst length, increment, AXI beat width and 2D stride. Fill them with
 * DMA4_XI_* from bcm2708.h. All fields little-endian.
 */
struct BCM2711DMA4CB
{
    ULONG   ti;             /* Transfer information (DMA4_TI_*) */
    ULONG   src;            /* Source address [31:0] */
    ULONG   srci;           /* Source address [39:32] plus attributes and stride */
    ULONG   dest;           /* Destination address [31:0] */
    ULONG   desti;          /* Destination address [39:32] plus attributes and stride */
    ULONG   len;            /* Transfer length (DMA4_LEN_*) */
    ULONG   next_cb;        /* Next control block address >> 5, 0 = stop */
    ULONG   reserved;
};

/* Channels 11-14 on the BCM2711 are DMA4; nothing on the other SoCs is. */
#define BCM2708_DMA_IS_DMA4(periiobase, ch) \
    (((periiobase) == BCM2708_DMA_PERIIOBASE_2711) && ((ch) >= 11) && ((ch) <= 14))

/*
 * What a DMA4 caller must know, all measured on a Pi 400 (v3d bring-up
 * 2026-08-23) - the datasheet alone is not enough:
 *
 * - SDRAM sits at bus 0x4_00000000 for DMA4 (the "large address" map,
 *   the same window the PCIe inbound BAR uses). A raw physical address
 *   points into the legacy/VPU alias space and the engine faults its CB
 *   fetch with READ_CB_ERROR. Use BCM2711_DMA4_SDRAM() on the source,
 *   destination AND control block addresses.
 *
 * - The kick value needs the PROT bits (CS[9:8]). The datasheet calls
 *   them reserved, but transfers kicked without them were rejected
 *   outright, so they belong in every DMA4 kick.
 *
 * - ERRATUM: a CB transfers at most low16(LEN) bytes. The documented
 *   30-bit linear length does not work on this silicon: LEN=0x100000
 *   moves 0 bytes and completes "successfully". Split transfers above
 *   60KB into multiple CBs.
 *
 * - WARNING: CB chains via next_cb have been observed to WANDER - the
 *   engine left a correctly built, cache-cleaned 160-link chain and
 *   executed unrelated RAM as control blocks ("if garbage is read then
 *   it will execute it"), spraying copies at random addresses. Cause
 *   not yet understood. Until it is, drive DMA4 with SINGLE CBs of
 *   <=60KB and kick each one separately.
 *
 * - TDMODE errors out instantly on DMA4, whatever the register layout
 *   suggests. Linear only.
 */
#define BCM2711_DMA4_SDRAM(x)   (0x400000000ULL | (UQUAD)(IPTR)(x))
#define BCM2711_DMA4_CS_PROT    (3UL << 8)
#define BCM2711_DMA4_CS_RUN     (BCM2708_DMA_CS_RUN | BCM2711_DMA4_CS_PROT)
#define BCM2711_DMA4_CS_ACK     (BCM2708_DMA_CS_ACK | BCM2711_DMA4_CS_PROT)
#define BCM2711_DMA4_MAX_LEN    (60 << 10)

/*
 * The uncached VideoCore bus alias covers the first gigabyte only. Above that
 * the cast drops the high bits and the engine reads elsewhere, so check first.
 * DMA4 has none of this: it takes a 40-bit physical address directly, so the
 * bus alias must not be applied to a DMA4 control block.
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
    /* The SPI each channel raises on the BCM2711, in channel order. The four
     * DMA4 engines get a line each again, so the sharing stops after 10. */
    static const UBYTE spi[] = { 80, 81, 82, 83, 84, 85, 86, 87, 87, 88, 88,
                                 89, 90, 91, 92 };

    if (periiobase != BCM2708_DMA_PERIIOBASE_2711)
        return BCM2708_DMA_IRQ_BASE + channel;

    if (channel < sizeof(spi) / sizeof(spi[0]))
        return 32 + spi[channel];   /* the GIC presents SPI n as INTID 32+n */

    /* Not a channel this SoC hands out; keep the arithmetic answer. */
    return BCM2708_DMA_IRQ_BASE + channel + BCM2708_DMA_GPUIRQ_OFFSET;
}

#endif /* HARDWARE_BCM2708_DMA_H */
