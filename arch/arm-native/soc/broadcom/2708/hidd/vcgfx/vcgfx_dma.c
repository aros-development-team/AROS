/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore4 Gfx Hidd - DMA-accelerated 2D operations.

    Uses a full BCM2835 DMA engine (allocated from dma.resource) in 2D
    stride mode for framebuffer copies and image transfers. The
    framebuffer is mapped uncached, so CPU reads from it are extremely
    slow — the DMA engine reads at memory speed, which is where the win
    comes from.

    All operations are synchronous: the caller (graphics.library)
    expects the pixels to be in place on return.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/dma.h>

#include <exec/memory.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hardware.h"
#include "vcgfx_neon.h"

extern APTR KernelBase;
APTR DMABase = NULL;

static inline void vc4_dsb(void) { asm volatile("dsb sy" ::: "memory"); }

/* BCM system timer ticks at 1 MHz. */
static inline ULONG vc4_now_us(void)
{
    return AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO);
}

/* 2D mode field limits: XLENGTH is 16 bits, YLENGTH 14 bits, the two
 * stride halves are signed 16-bit. */
#define VC4_DMA_MAX_XLEN    0xFFFF
#define VC4_DMA_MAX_YLEN    0x3FFF
#define VC4_DMA_MAX_STRIDE  0x7FFF

/* Above the first gigabyte BCM2708_DMA_BUS_ADDR() drops the high bits and
 * the transfer lands elsewhere - reachable on a Pi 4, never on a 1GB
 * board. IPTR, not ULONG: that cast would itself hide a >4GB overflow.
 * Out of range is never fatal, callers fall back to NEON. */
static inline BOOL vc4_dma_addressable(IPTR phys, ULONG bytes)
{
    return BCM2708_DMA_ADDRESSABLE(phys)
        && BCM2708_DMA_ADDRESSABLE(phys + bytes);
}

int FNAME_SUPPORT(InitDMA)(struct VideoCoreGfx_staticdata *xsd)
{
    APTR raw;

    xsd->vcsd_DMAChannel = -1;
    InitSemaphore(&xsd->vcsd_DMALock);

    if ((DMABase = OpenResource("dma.resource")) == NULL)
    {
        bug("[VideoCoreGfx] no DMA: dma.resource missing\n");
        return FALSE;
    }

    /* CB (32 bytes, 32-byte aligned) + the fill source pixel directly
     * behind it, so one cache flush covers both. */
    if ((raw = AllocVec(sizeof(struct BCM2708DMACB) + 32 + sizeof(ULONG),
                        MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        return FALSE;

    xsd->vcsd_DMACBRaw = raw;
    xsd->vcsd_DMACB = (struct BCM2708DMACB *)(((IPTR)raw + 31) & ~31);
    xsd->vcsd_DMAFillPx = (ULONG *)(xsd->vcsd_DMACB + 1);

    /* The engine fetches the CB through the alias too, so an unreachable
     * one costs the whole channel. Covers the fill pixel behind it. */
    if (!vc4_dma_addressable((IPTR)KrnVirtualToPhysical(xsd->vcsd_DMACB),
                             sizeof(struct BCM2708DMACB) + sizeof(ULONG)))
    {
        bug("[VideoCoreGfx] no DMA: control block at phys 0x%p is above the "
            "1GB bus alias\n", KrnVirtualToPhysical(xsd->vcsd_DMACB));
        FreeVec(raw);
        xsd->vcsd_DMACBRaw = NULL;
        return FALSE;
    }

    if ((xsd->vcsd_DMAChannel = DMAAllocChannel(DMACHF_TDMODE | DMACHF_IRQ)) < 0)
    {
        bug("[VideoCoreGfx] no DMA: no 2D-capable channel free\n");
        FreeVec(raw);
        xsd->vcsd_DMACBRaw = NULL;
        return FALSE;
    }

    /* Bounce buffer for DMA reads (GetImage). DMA must not write into
     * cached caller memory — an edge cache line could be written back over
     * the DMA'd data. The bounce is 32-byte aligned and ours alone, so a
     * pre-DMA clean+invalidate makes it safe. Optional: without it
     * vc4_dma_get returns FALSE and the NEON path runs. */
    if ((xsd->vcsd_DMABounceRaw = AllocVec(VC4_DMA_BOUNCE_SIZE + 31,
                                           MEMF_PUBLIC)) != NULL)
    {
        UBYTE *bounce = (UBYTE *)(((IPTR)xsd->vcsd_DMABounceRaw + 31) & ~31);
        IPTR phys = (IPTR)KrnVirtualToPhysical(bounce);

        if (vc4_dma_addressable(phys, VC4_DMA_BOUNCE_SIZE))
        {
            xsd->vcsd_DMABounce = bounce;
            xsd->vcsd_DMABouncePhys = (ULONG)phys;
        }
        else
        {
            /* Reads keep working, just on the NEON path. */
            D(bug("[VideoCoreGfx] %s: bounce buffer above the 1GB alias\n",
                __PRETTY_FUNCTION__));
            FreeVec(xsd->vcsd_DMABounceRaw);
            xsd->vcsd_DMABounceRaw = NULL;
        }
    }

    bug("[VideoCoreGfx] DMA channel %d, CB at phys 0x%p, bounce %s\n",
        (int)xsd->vcsd_DMAChannel, KrnVirtualToPhysical(xsd->vcsd_DMACB),
        xsd->vcsd_DMABounce ? "ready" : "unavailable");
    return TRUE;
}

/*
 * A flat deadline does not work: a full-window scroll moves ~4 MB each way,
 * which takes on the order of 100 ms once the HVS competes for the SDRAM.
 * Budget a pessimistic 25 MB/s, capped so a wedged channel is noticed.
 *
 * A contiguous copy goes out 128 bits at a time (see vc4_dma_copy) and
 * beats that comfortably, but the budget stays pessimistic: it only has to
 * be generous enough that a genuinely wedged channel is still noticed.
 */
static ULONG vc4_dma_timeout_us(ULONG bytes)
{
    ULONG us = 50000 + bytes / 25;

    return us > 300000 ? 300000 : us;
}

/* Kick the prepared CB and wait for completion. Must be called with
 * vcsd_DMALock held. Returns FALSE (after a channel reset) on timeout. */
static BOOL vc4_dma_run(struct VideoCoreGfx_staticdata *xsd, const char *op,
                        ULONG bytes)
{
    volatile ULONG *dma_cs = (volatile ULONG *)DMA_CS(xsd->vcsd_DMAChannel);
    volatile ULONG *dma_cb = (volatile ULONG *)DMA_CONBLK_AD(xsd->vcsd_DMAChannel);
    ULONG cb_phys = (ULONG)(IPTR)KrnVirtualToPhysical(xsd->vcsd_DMACB);
    ULONG start;

    /* Diagnostic: one line per DMA use, CB fields in host byte order. */
    D(bug("[VideoCoreGfx] DMA %s ch=%d ti=0x%08x len=0x%08x stride=0x%08x "
        "src=0x%08x dst=0x%08x\n", op, (int)xsd->vcsd_DMAChannel,
        AROS_LE2LONG(xsd->vcsd_DMACB->ti),
        AROS_LE2LONG(xsd->vcsd_DMACB->txfr_len),
        AROS_LE2LONG(xsd->vcsd_DMACB->stride),
        AROS_LE2LONG(xsd->vcsd_DMACB->source_ad),
        AROS_LE2LONG(xsd->vcsd_DMACB->dest_ad)));

    CacheClearE(xsd->vcsd_DMACB, sizeof(struct BCM2708DMACB) + sizeof(ULONG),
                CACRF_ClearD);
    vc4_dsb();

    *dma_cs = AROS_LONG2LE(DMA_CS_INT | DMA_CS_END);
    *dma_cb = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(cb_phys));
    vc4_dsb();
    *dma_cs = AROS_LONG2LE(
        DMA_CS_WAIT_FOR_WRITES |
        DMA_CS_PANIC_PRI(15) |
        DMA_CS_PRI(8) |
        DMA_CS_ACTIVE);

    /* IRQ-driven wait via dma.resource (INTEN set in the CBs): sleeps the
     * task, handles the wedge-safe timeout, resets the channel on failure. */
    if (DMAWaitChannel(xsd->vcsd_DMAChannel, vc4_dma_timeout_us(bytes)) == 0)
        return TRUE;

    /* Unconditional - rare, and the damage is visible. CS/DEBUG come from
     * dma.resource, which samples them before resetting the channel. */
    bug("[VideoCoreGfx] DMA %s failed after %uus: "
        "ti=0x%08x len=0x%08x stride=0x%08x src=0x%08x dst=0x%08x\n",
        op, (unsigned)vc4_dma_timeout_us(bytes),
        AROS_LE2LONG(xsd->vcsd_DMACB->ti),
        AROS_LE2LONG(xsd->vcsd_DMACB->txfr_len),
        AROS_LE2LONG(xsd->vcsd_DMACB->stride),
        AROS_LE2LONG(xsd->vcsd_DMACB->source_ad),
        AROS_LE2LONG(xsd->vcsd_DMACB->dest_ad));
    return FALSE;
}

/* One column band of a rectangle copy. bottom_up walks rows last-to-first
 * via negative strides - required when src/dest overlap with the
 * destination below the source. */
static BOOL vc4_dma_copy_band(struct VideoCoreGfx_staticdata *xsd,
                              ULONG src_phys, ULONG src_pitch,
                              ULONG dst_phys, ULONG dst_pitch,
                              ULONG width_bytes, ULONG height,
                              BOOL bottom_up)
{
    struct BCM2708DMACB *cb;
    LONG s_stride, d_stride;
    BOOL ok;

    if (xsd->vcsd_DMAChannel < 0 || width_bytes == 0 || height == 0)
        return FALSE;
    if (width_bytes > VC4_DMA_MAX_XLEN || (height - 1) > VC4_DMA_MAX_YLEN)
        return FALSE;

    /* Check before bottom_up moves the bases to the last row. */
    if (!vc4_dma_addressable(src_phys, (height - 1) * src_pitch + width_bytes)
        || !vc4_dma_addressable(dst_phys, (height - 1) * dst_pitch + width_bytes))
        return FALSE;

    if (bottom_up)
    {
        src_phys += (height - 1) * src_pitch;
        dst_phys += (height - 1) * dst_pitch;
        s_stride = -(LONG)(src_pitch + width_bytes);
        d_stride = -(LONG)(dst_pitch + width_bytes);
    }
    else
    {
        s_stride = (LONG)(src_pitch - width_bytes);
        d_stride = (LONG)(dst_pitch - width_bytes);
    }
    if (s_stride < -VC4_DMA_MAX_STRIDE || s_stride > VC4_DMA_MAX_STRIDE ||
        d_stride < -VC4_DMA_MAX_STRIDE || d_stride > VC4_DMA_MAX_STRIDE)
        return FALSE;

    ObtainSemaphore(&xsd->vcsd_DMALock);

    cb = xsd->vcsd_DMACB;
    cb->ti = AROS_LONG2LE(DMA_TI_INTEN | DMA_TI_SRC_INC | DMA_TI_DEST_INC |
                          DMA_TI_BURST_LENGTH(8) | DMA_TI_TDMODE);
    cb->source_ad = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(src_phys));
    cb->dest_ad   = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(dst_phys));
    cb->txfr_len  = AROS_LONG2LE(DMA_TXFR_LEN_2D(width_bytes, height - 1));
    cb->stride    = AROS_LONG2LE(DMA_STRIDE_2D((UWORD)s_stride, (UWORD)d_stride));
    cb->nextconbk = 0;
    cb->reserved[0] = 0;
    cb->reserved[1] = 0;

    ok = vc4_dma_run(xsd, bottom_up ? "copy^" : "copy",
                     width_bytes * height);

    ReleaseSemaphore(&xsd->vcsd_DMALock);
    return ok;
}

/* A contiguous run: one linear transfer, so there is no per-row address
 * arithmetic for a wide transfer to get wrong. */
static BOOL vc4_dma_copy_linear(struct VideoCoreGfx_staticdata *xsd,
                                ULONG src_phys, ULONG dst_phys,
                                ULONG bytes, ULONG wide)
{
    struct BCM2708DMACB *cb;
    BOOL ok;

    if (bytes == 0)
        return TRUE;

    ObtainSemaphore(&xsd->vcsd_DMALock);

    cb = xsd->vcsd_DMACB;
    cb->ti = AROS_LONG2LE(DMA_TI_INTEN | DMA_TI_SRC_INC | DMA_TI_DEST_INC |
                          DMA_TI_BURST_LENGTH(8) | wide);
    cb->source_ad = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(src_phys));
    cb->dest_ad   = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(dst_phys));
    cb->txfr_len  = AROS_LONG2LE(bytes);    /* 1D: 30-bit byte count */
    cb->stride    = 0;
    cb->nextconbk = 0;
    cb->reserved[0] = 0;
    cb->reserved[1] = 0;

    ok = vc4_dma_run(xsd, wide ? "copy16" : "copy4", bytes);

    ReleaseSemaphore(&xsd->vcsd_DMALock);
    return ok;
}

/*
 * 2D rectangle copy between two uncached physical buffers (FB regions).
 *
 * 128-bit transfers move several times the bytes per cycle that the
 * default 32-bit ones do, which is most of the cost of scrolling a window.
 * They are only taken where the whole rectangle spans full rows of both
 * buffers, though: that is one contiguous run, so it goes out as a single
 * linear transfer with no stride at all, and the only unaligned remainders
 * are a few bytes at each end of the run rather than of every row.
 *
 * Splitting a strided rectangle into columns and widening the middle was
 * tried and is measurably faster, but it crashed the machine - a wide
 * transfer with a non-zero stride is a combination this engine had never
 * been asked for here. Until that is understood, anything strided stays
 * 32-bit, which is exactly what it was before.
 */
BOOL vc4_dma_copy(struct VideoCoreGfx_staticdata *xsd,
                  ULONG src_phys, ULONG src_pitch,
                  ULONG dst_phys, ULONG dst_pitch,
                  ULONG width_bytes, ULONG height, BOOL bottom_up)
{
    /* bottom_up exists to walk rows backwards over an overlap, which a
     * linear run cannot do - but it only arises when the destination is
     * below the source, and a terminal scrolls the other way. */
    if (!bottom_up && width_bytes && height
        && src_pitch == width_bytes && dst_pitch == width_bytes
        && ((src_phys ^ dst_phys) & 15) == 0)
    {
        ULONG total = width_bytes * height;

        if (vc4_dma_addressable(src_phys, total)
            && vc4_dma_addressable(dst_phys, total))
        {
            ULONG lead = (16 - (src_phys & 15)) & 15;
            ULONG mid;

            if (lead > total)
                lead = total;
            mid = (total - lead) & ~15u;

            if (mid)
            {
                return vc4_dma_copy_linear(xsd, src_phys, dst_phys, lead, 0)
                    && vc4_dma_copy_linear(xsd, src_phys + lead,
                                           dst_phys + lead, mid,
                                           DMA_TI_SRC_WIDTH | DMA_TI_DEST_WIDTH)
                    && vc4_dma_copy_linear(xsd, src_phys + lead + mid,
                                           dst_phys + lead + mid,
                                           total - lead - mid, 0);
            }
        }
    }

    return vc4_dma_copy_band(xsd, src_phys, src_pitch, dst_phys, dst_pitch,
                             width_bytes, height, bottom_up);
}

/*
 * Rectangle copy from cached RAM (PutImage source) into an uncached
 * physical buffer. Source rows are cleaned to RAM first so the engine,
 * reading through the uncached alias, sees the CPU's latest data. RAM is
 * identity-mapped here, so rows are physically contiguous.
 */
BOOL vc4_dma_put(struct VideoCoreGfx_staticdata *xsd,
                 const UBYTE *src, ULONG src_modulo,
                 ULONG dst_phys, ULONG dst_pitch,
                 ULONG width_bytes, ULONG height)
{
    struct BCM2708DMACB *cb;
    LONG s_stride = (LONG)(src_modulo - width_bytes);
    LONG d_stride = (LONG)(dst_pitch - width_bytes);
    IPTR src_phys;
    BOOL ok;

    if (xsd->vcsd_DMAChannel < 0 || width_bytes == 0 || height == 0)
        return FALSE;
    if (width_bytes > VC4_DMA_MAX_XLEN || (height - 1) > VC4_DMA_MAX_YLEN ||
        s_stride < 0 || s_stride > VC4_DMA_MAX_STRIDE ||
        d_stride < 0 || d_stride > VC4_DMA_MAX_STRIDE)
        return FALSE;

    /* An ordinary bitmap - the one source that can sit anywhere in RAM. */
    src_phys = (IPTR)KrnVirtualToPhysical((APTR)src);
    if (!vc4_dma_addressable(src_phys, (height - 1) * src_modulo + width_bytes)
        || !vc4_dma_addressable(dst_phys, (height - 1) * dst_pitch + width_bytes))
        return FALSE;

    if (src_modulo == width_bytes)
        CacheClearE((APTR)src, height * width_bytes, CACRF_ClearD);
    else
    {
        ULONG y;
        for (y = 0; y < height; y++)
            CacheClearE((APTR)(src + y * src_modulo), width_bytes, CACRF_ClearD);
    }
    vc4_dsb();

    ObtainSemaphore(&xsd->vcsd_DMALock);

    cb = xsd->vcsd_DMACB;
    cb->ti = AROS_LONG2LE(DMA_TI_INTEN | DMA_TI_SRC_INC | DMA_TI_DEST_INC |
                          DMA_TI_BURST_LENGTH(8) | DMA_TI_TDMODE);
    cb->source_ad = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(src_phys));
    cb->dest_ad   = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(dst_phys));
    cb->txfr_len  = AROS_LONG2LE(DMA_TXFR_LEN_2D(width_bytes, height - 1));
    cb->stride    = AROS_LONG2LE(DMA_STRIDE_2D((UWORD)s_stride, (UWORD)d_stride));
    cb->nextconbk = 0;
    cb->reserved[0] = 0;
    cb->reserved[1] = 0;

    ok = vc4_dma_run(xsd, "put", width_bytes * height);

    ReleaseSemaphore(&xsd->vcsd_DMALock);
    return ok;
}

/*
 * Rectangle copy from an uncached physical buffer into cached RAM (GetImage
 * destination). DMA lands in the private bounce buffer and NEON copies rows
 * out, avoiding DMA writes into cache lines the caller may touch.
 */
BOOL vc4_dma_get(struct VideoCoreGfx_staticdata *xsd,
                 ULONG src_phys, ULONG src_pitch,
                 UBYTE *dst, ULONG dst_modulo,
                 ULONG width_bytes, ULONG height)
{
    LONG s_stride = (LONG)(src_pitch - width_bytes);
    ULONG rows_per_chunk, done = 0;
    BOOL ok = TRUE;

    if (xsd->vcsd_DMAChannel < 0 || !xsd->vcsd_DMABounce ||
        width_bytes == 0 || height == 0)
        return FALSE;
    if (width_bytes > VC4_DMA_BOUNCE_SIZE ||
        s_stride < 0 || s_stride > VC4_DMA_MAX_STRIDE)
        return FALSE;
    /* The bounce buffer was checked once at init. */
    if (!vc4_dma_addressable(src_phys, (height - 1) * src_pitch + width_bytes))
        return FALSE;

    rows_per_chunk = VC4_DMA_BOUNCE_SIZE / width_bytes;
    if (rows_per_chunk > VC4_DMA_MAX_YLEN)
        rows_per_chunk = VC4_DMA_MAX_YLEN;

    ObtainSemaphore(&xsd->vcsd_DMALock);

    while (done < height && ok)
    {
        ULONG n = height - done;
        struct BCM2708DMACB *cb = xsd->vcsd_DMACB;
        ULONG y;

        if (n > rows_per_chunk)
            n = rows_per_chunk;

        /* Evict the bounce region (including clean lines cached by the
         * previous chunk's copy-out) before the engine writes it. */
        CacheClearE(xsd->vcsd_DMABounce, n * width_bytes, CACRF_ClearD);
        vc4_dsb();

        cb->ti = AROS_LONG2LE(DMA_TI_INTEN | DMA_TI_SRC_INC | DMA_TI_DEST_INC |
                              DMA_TI_BURST_LENGTH(8) | DMA_TI_TDMODE);
        cb->source_ad = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(
            src_phys + done * src_pitch));
        cb->dest_ad   = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(
            xsd->vcsd_DMABouncePhys));
        cb->txfr_len  = AROS_LONG2LE(DMA_TXFR_LEN_2D(width_bytes, n - 1));
        cb->stride    = AROS_LONG2LE(DMA_STRIDE_2D((UWORD)s_stride, 0));
        cb->nextconbk = 0;
        cb->reserved[0] = 0;
        cb->reserved[1] = 0;

        ok = vc4_dma_run(xsd, "get", width_bytes * n);

        if (ok)
        {
            /* Invalidate again post-DMA: the CPU may have speculatively
             * prefetched bounce lines while the engine wrote them, leaving
             * them stale (seen on Pi 3 as 8-pixel stripes of old data).
             * Lines are clean here (CPU only reads the bounce), so
             * invalidate-only is safe. */
            CacheClearE(xsd->vcsd_DMABounce, n * width_bytes,
                        CACRF_InvalidateD);
            vc4_dsb();

            for (y = 0; y < n; y++)
                neon_copyline(dst + (done + y) * dst_modulo,
                              xsd->vcsd_DMABounce + y * width_bytes,
                              width_bytes);
            done += n;
        }
    }

    ReleaseSemaphore(&xsd->vcsd_DMALock);
    return ok;
}

/*
 * 2D rectangle fill of a physically-addressed uncached buffer. The
 * engine reads the pixel value from a single non-incrementing source
 * word and streams it across the rectangle.
 */
BOOL vc4_dma_fill(struct VideoCoreGfx_staticdata *xsd,
                  ULONG dst_phys, ULONG dst_pitch,
                  ULONG width_bytes, ULONG height, ULONG pixel)
{
    struct BCM2708DMACB *cb;
    LONG d_stride = (LONG)(dst_pitch - width_bytes);
    BOOL ok;

    if (xsd->vcsd_DMAChannel < 0 || width_bytes == 0 || height == 0)
        return FALSE;
    if (width_bytes > VC4_DMA_MAX_XLEN || (height - 1) > VC4_DMA_MAX_YLEN ||
        d_stride > VC4_DMA_MAX_STRIDE)
        return FALSE;
    /* The fill pixel lives behind the CB, checked once at init. */
    if (!vc4_dma_addressable(dst_phys, (height - 1) * dst_pitch + width_bytes))
        return FALSE;

    ObtainSemaphore(&xsd->vcsd_DMALock);

    *xsd->vcsd_DMAFillPx = pixel;

    cb = xsd->vcsd_DMACB;
    cb->ti = AROS_LONG2LE(DMA_TI_DEST_INC |
                          DMA_TI_WAIT_RESP | DMA_TI_TDMODE);
    cb->source_ad = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(
        (ULONG)(IPTR)KrnVirtualToPhysical((APTR)xsd->vcsd_DMAFillPx)));
    cb->dest_ad   = AROS_LONG2LE(BCM2708_DMA_BUS_ADDR(dst_phys));
    cb->txfr_len  = AROS_LONG2LE(DMA_TXFR_LEN_2D(width_bytes, height - 1));
    cb->stride    = AROS_LONG2LE(DMA_STRIDE_2D(0, (UWORD)d_stride));
    cb->nextconbk = 0;
    cb->reserved[0] = 0;
    cb->reserved[1] = 0;

    ok = vc4_dma_run(xsd, "fill", width_bytes * height);

    ReleaseSemaphore(&xsd->vcsd_DMALock);
    return ok;
}

