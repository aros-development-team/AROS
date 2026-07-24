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

#include "vc4gfx_hidd.h"
#include "vc4gfx_hardware.h"
#include "vc4gfx_neon.h"

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

int FNAME_SUPPORT(InitDMA)(struct VideoCoreGfx_staticdata *xsd)
{
    APTR raw;

    xsd->vcsd_DMAChannel = -1;
    InitSemaphore(&xsd->vcsd_DMALock);

    if ((DMABase = OpenResource("dma.resource")) == NULL)
        return FALSE;

    /* CB (32 bytes, 32-byte aligned) + the fill source pixel directly
     * behind it, so one cache flush covers both. */
    if ((raw = AllocVec(sizeof(struct BCM2708DMACB) + 32 + sizeof(ULONG),
                        MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        return FALSE;

    xsd->vcsd_DMACBRaw = raw;
    xsd->vcsd_DMACB = (struct BCM2708DMACB *)(((IPTR)raw + 31) & ~31);
    xsd->vcsd_DMAFillPx = (ULONG *)(xsd->vcsd_DMACB + 1);

    if ((xsd->vcsd_DMAChannel = DMAAllocChannel(DMACHF_TDMODE | DMACHF_IRQ)) < 0)
    {
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
        xsd->vcsd_DMABounce =
            (UBYTE *)(((IPTR)xsd->vcsd_DMABounceRaw + 31) & ~31);
        xsd->vcsd_DMABouncePhys =
            (ULONG)(IPTR)KrnVirtualToPhysical(xsd->vcsd_DMABounce);
    }

    D(bug("[VideoCoreGfx] %s: DMA channel %d\n", __PRETTY_FUNCTION__,
        (int)xsd->vcsd_DMAChannel));
    return TRUE;
}

/* Kick the prepared CB and wait for completion. Must be called with
 * vcsd_DMALock held. Returns FALSE (after a channel reset) on timeout. */
static BOOL vc4_dma_run(struct VideoCoreGfx_staticdata *xsd, const char *op)
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
    if (DMAWaitChannel(xsd->vcsd_DMAChannel, 100000) == 0)
        return TRUE;

    bug("[VideoCoreGfx] DMA %s timeout\n", op);
    return FALSE;
}

/*
 * 2D rectangle copy between two uncached physical buffers (FB regions).
 * bottom_up walks rows last-to-first via negative strides — required when
 * src/dest overlap with the destination below the source.
 */
BOOL vc4_dma_copy(struct VideoCoreGfx_staticdata *xsd,
                  ULONG src_phys, ULONG src_pitch,
                  ULONG dst_phys, ULONG dst_pitch,
                  ULONG width_bytes, ULONG height, BOOL bottom_up)
{
    struct BCM2708DMACB *cb;
    LONG s_stride, d_stride;
    BOOL ok;

    if (xsd->vcsd_DMAChannel < 0 || width_bytes == 0 || height == 0)
        return FALSE;
    if (width_bytes > VC4_DMA_MAX_XLEN || (height - 1) > VC4_DMA_MAX_YLEN)
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

    ok = vc4_dma_run(xsd, bottom_up ? "copy^" : "copy");

    ReleaseSemaphore(&xsd->vcsd_DMALock);
    return ok;
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
    ULONG src_phys;
    BOOL ok;

    if (xsd->vcsd_DMAChannel < 0 || width_bytes == 0 || height == 0)
        return FALSE;
    if (width_bytes > VC4_DMA_MAX_XLEN || (height - 1) > VC4_DMA_MAX_YLEN ||
        s_stride < 0 || s_stride > VC4_DMA_MAX_STRIDE ||
        d_stride < 0 || d_stride > VC4_DMA_MAX_STRIDE)
        return FALSE;

    src_phys = (ULONG)(IPTR)KrnVirtualToPhysical((APTR)src);

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

    ok = vc4_dma_run(xsd, "put");

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

        ok = vc4_dma_run(xsd, "get");

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

    ok = vc4_dma_run(xsd, "fill");

    ReleaseSemaphore(&xsd->vcsd_DMALock);
    return ok;
}
