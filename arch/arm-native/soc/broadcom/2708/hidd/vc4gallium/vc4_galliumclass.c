/*
    Copyright 2025, The AROS Development Team. All rights reserved.

    VC4 Gallium 3D HIDD - Gallium pipe_screen interface

    Implements the Hidd_Gallium interface for VideoCore IV.
    Mesa3DGL.library creates this HIDD and calls CreatePipeScreen()
    to get a Gallium pipe_screen. DisplayResource() blits rendered
    frames to the AROS display.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/dma.h>

#include <cybergraphx/cybergraphics.h>
#include <string.h>

#include <hidd/gallium.h>
#include <hidd/gfx.h>
#include <gallium/gallium.h>

#include <aros/macros.h>

#include "pipe/p_state.h"
#include "pipe/p_screen.h"

#include "vc4gallium_intern.h"
#include "vc4_drm_aros.h"
#include "vc4_v3d.h"  /* provides ARM_PERIIOBASE, bcm2708.h, GPU_BUS_ADDR */

/* vc4gfx bitmap attributes — must match the aoHidd_VideoCoreGfxBitMap_*
 * enum in vc4gfx_bitmap.h */
#define aoHidd_VideoCoreGfxBitMap_Drawable      0
#define aoHidd_VideoCoreGfxBitMap_BackDrawable  1
#define aoHidd_VideoCoreGfxBitMap_Flip          2
#define aoHidd_VideoCoreGfxBitMap_Overlay       3

/* Mirrored from vc4gfx_bitmap.h, like the attr indices above. */
struct vc4gfx_overlay
{
    ULONG ovl_Phys;
    ULONG ovl_Pitch;
    ULONG ovl_Width, ovl_Height;
    LONG  ovl_X, ovl_Y;
    ULONG ovl_DestW, ovl_DestH;     /* 0/== source = unscaled */
};

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT RECTFMT_RAW
#else
#define AROS_PIXFMT RECTFMT_BGRA32
#endif

#define UtilityBase (SD(cl)->UtilityBase)

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase (SD(cl)->hiddGalliumAB)

#undef HiddBitMapAttrBase
#define HiddBitMapAttrBase (SD(cl)->hiddBitMapAB)

/* ---- DMA helpers ---- */

static inline void __gallium_dsb(void) { asm volatile("dsb sy" ::: "memory"); }

/* BCM system timer ticks at 1 MHz — use it for real microsecond budgets
 * instead of CPU-clock-dependent spin counts. */
static inline ULONG gallium_now_us(void)
{
    return AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO);
}

static inline void gallium_udelay(ULONG usec)
{
    ULONG start = gallium_now_us();
    while ((gallium_now_us() - start) < usec)
        asm volatile("nop");
}

/*
 * Scheduler-friendly microsleep for the GPU wait loops: a timer.device
 * UNIT_MICROHZ request lets other tasks (input, mouse) run while we wait
 * out a slice of the render. Falls back to busy-wait if the timer was
 * unavailable at init. The pre-opened device/unit is cloned into a stack
 * request so any task may call this.
 */
void vc4_gpu_nap(struct vc4galliumstaticdata *sd, ULONG us)
{
    struct MsgPort port;
    struct timerequest tr;
    BYTE sig;

    if (!sd->gpu_timer_ok || (sig = AllocSignal(-1)) < 0)
    {
        gallium_udelay(us);
        return;
    }

    /* Zero the whole port: on SMP-platform builds struct MsgPort carries
     * a spinlock, and stack garbage in it reads as "locked" — the timer
     * interrupt's ReplyMsg then spins forever in interrupt context,
     * hanging the machine. Zero == unlocked. */
    memset(&port, 0, sizeof(port));
    port.mp_Node.ln_Type = NT_MSGPORT;
    port.mp_Flags        = PA_SIGNAL;
    port.mp_SigBit       = sig;
    port.mp_SigTask      = FindTask(NULL);
    NEWLIST(&port.mp_MsgList);

    tr = sd->gpu_timer_template;
    tr.tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    tr.tr_node.io_Message.mn_ReplyPort = &port;
    tr.tr_node.io_Message.mn_Length = sizeof(tr);
    tr.tr_node.io_Command = TR_ADDREQUEST;
    tr.tr_time.tv_secs  = 0;
    tr.tr_time.tv_micro = us;

    DoIO((struct IORequest *)&tr);

    FreeSignal(sig);
}

/*
 * Wait for any in-progress display-blit DMA via the dma.resource
 * completion-IRQ wait (the CB sets DMA_TI_INTEN); the resource handles the
 * wedge-safe timeout and channel reset. Non-static so the BO teardown path
 * in vc4_drm_aros.c can drain DMA before releasing dma_pinned_handle.
 */
void vc4_aros_dma_wait_idle(struct vc4galliumstaticdata *sd)
{
    if (!sd->dmaBusy)
        return;

    if (DMAWaitChannel(sd->dmaChannel, 1000000) != 0)
        bug("[VC4Gallium] DMA timeout!\n");

    sd->dmaBusy = FALSE;
}

/*
 * Direct 2D stride DMA blit from GPU BO to framebuffer.
 * Uses a single control block in TDMODE — no staging buffer, no CPU copy.
 * Both source and destination are physically addressable on RPi.
 * Waits for completion before returning (synchronous for display).
 */
static BOOL gallium_dma_blit_direct(struct vc4galliumstaticdata *sd,
                                     ULONG src_phys, ULONG src_pitch,
                                     ULONG dst_phys, ULONG dst_pitch,
                                     ULONG width_bytes, ULONG height)
{
    ULONG cb_needed = sizeof(struct BCM2708DMACB) + 31;
    struct BCM2708DMACB *cb;
    ULONG cb_bus;
    volatile ULONG *dma_cs;
    volatile ULONG *dma_cb_reg;

    if (height == 0 || width_bytes == 0 || sd->dmaChannel < 0)
        return FALSE;

    vc4_aros_dma_wait_idle(sd);

    /* Ensure CB buffer exists (one CB, allocated once) */
    if (sd->dmaCBRawSize < cb_needed)
    {
        if (sd->dmaCBRaw)
            FreeMem(sd->dmaCBRaw, sd->dmaCBRawSize);
        sd->dmaCBRaw = AllocMem(cb_needed, MEMF_PUBLIC);
        if (!sd->dmaCBRaw)
        {
            sd->dmaCBRawSize = 0;
            return FALSE;
        }
        sd->dmaCBRawSize = cb_needed;
    }

    cb = (struct BCM2708DMACB *)(((IPTR)sd->dmaCBRaw + 31) & ~31);

    /* 2D stride mode: single CB transfers the entire rectangle.
     * txfr_len = YLENGTH(height) | XLENGTH(width_bytes)
     * stride   = D_STRIDE | S_STRIDE (signed 16-bit each).
     * BURST_LENGTH(8) + wide bursts (default, BO/FB are 16-byte aligned)
     * are essential for throughput: without a burst length the engine
     * issues one AXI beat at a time, and WAIT_RESP then stalls on each
     * write response — measured at ~30 MB/s. WAIT_RESP is dropped; the
     * DMA_CS_WAIT_FOR_WRITES bit in the kick already drains outstanding
     * writes before the channel signals END, so completion is safe. */
    cb->ti = AROS_LONG2LE(DMA_TI_INTEN |
                           DMA_TI_SRC_INC | DMA_TI_DEST_INC |
                           DMA_TI_BURST_LENGTH(8) | DMA_TI_TDMODE);
    cb->source_ad = AROS_LONG2LE(GPU_BUS_ADDR(src_phys));
    cb->dest_ad = AROS_LONG2LE(GPU_BUS_ADDR(dst_phys));
    cb->txfr_len = AROS_LONG2LE(((height - 1) << 16) | width_bytes);
    cb->stride = AROS_LONG2LE(
        ((UWORD)(dst_pitch - width_bytes) << 16) |
        (UWORD)(src_pitch - width_bytes));
    cb->nextconbk = 0;
    cb->reserved[0] = 0;
    cb->reserved[1] = 0;

    /* No source cache flush: the DMA reads the BO via the uncached 0xC alias,
     * the GPU (not CPU) produced the pixels, and vc4_aros_wait_idle() already
     * stored V3D's tiles to RAM. A previous per-row CacheClearE here (copied
     * from a CPU-read path) dominated the blit: ~143ms/1280x1024 frame vs
     * ~10ms for the transfer.
     *
     * Only the control block needs flushing — the CPU wrote it into cached
     * AllocMem and the DMA fetches it via the uncached alias. */
    CacheClearE(cb, sizeof(*cb), CACRF_ClearD);
    __gallium_dsb();

    /* CB lives in AllocMem memory — translate CPU VA to physical bus addr. */
    cb_bus = GPU_BUS_ADDR((ULONG)(IPTR)KrnVirtualToPhysical(cb));

    /* Start DMA. vc4_aros_dma_wait_idle() above left the channel idle; channel
     * reset is only needed on the timeout recovery path, not every kick. */
    dma_cs = (volatile ULONG *)DMA_CS(sd->dmaChannel);
    dma_cb_reg = (volatile ULONG *)DMA_CONBLK_AD(sd->dmaChannel);

    *dma_cs = AROS_LONG2LE(DMA_CS_INT | DMA_CS_END);
    *dma_cb_reg = AROS_LONG2LE(cb_bus);
    __gallium_dsb();
    *dma_cs = AROS_LONG2LE(
        DMA_CS_WAIT_FOR_WRITES |
        DMA_CS_PANIC_PRI(15) |
        DMA_CS_PRI(8) |
        DMA_CS_ACTIVE);

    sd->dmaBusy = TRUE;

    /* DMA runs asynchronously — the next vc4_aros_dma_wait_idle() or
     * gallium_dma_blit_direct() waits for completion. Lets the CPU start the
     * next frame while the engine transfers pixels to the framebuffer. */

    return TRUE;
}

/* ---- Gallium HIDD Methods ---- */

OOP_Object *METHOD(HiddVC4Gallium, Root, New)
{
    IPTR interfaceVers;

    D(bug("[VC4Gallium] %s()\n", __PRETTY_FUNCTION__));

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
    {
        bug("[VC4Gallium] Interface version mismatch: got %ld, expected %d\n",
            interfaceVers, GALLIUM_INTERFACE_VERSION);
        return NULL;
    }

    /* mesa3dgl's compiler-core table; one per system, so staticdata. The
     * driver's trampolines bind to it at CreatePipeScreen (and refuse a
     * table from a different Mesa generation there). */
    SD(cl)->coreapi = (APTR)GetTagData(aHidd_Gallium_CoreAPI, 0, msg->attrList);

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HiddGalliumVC4Data *data = OOP_INST_DATA(cl, o);
        data->vc4_obj = o;

        if (!SD(cl)->v3d.v3d_available)
        {
            bug("[VC4Gallium] V3D hardware not available\n");
            OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
            return NULL;
        }
    }

    return o;
}

VOID METHOD(HiddVC4Gallium, Root, Dispose)
{
    D(bug("[VC4Gallium] %s()\n", __PRETTY_FUNCTION__));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(HiddVC4Gallium, Root, Get)
{
    ULONG idx;

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gallium_InterfaceVersion:
            *msg->storage = GALLIUM_INTERFACE_VERSION;
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(HiddVC4Gallium, Hidd_Gallium, CreatePipeScreen)
{
    struct vc4galliumstaticdata *sd = SD(cl);

    /* Diagnostic: has the firmware governor re-clocked anything since
     * InitLib forced V3D to max? Logged once per GL context start. */
    vc4_log_clocks(sd, "CreatePipeScreen");

    if (!sd->coreapi)
    {
        bug("[VC4Gallium] no GalliumCoreAPI table (old mesa3dgl?)\n");
        return NULL;
    }

    if (!sd->bridge_inited)
    {
        vc4_aros_bridge_init(&sd->bridge, sd);
        sd->bridge_inited = TRUE;
    }

    return vc4_aros_create_screen(&sd->bridge, sd->coreapi);
}

IPTR METHOD(HiddVC4Gallium, Hidd_Gallium, DisplayResourceRP)
{
    return vc4_aros_display_rp(msg->resource, msg->srcx, msg->srcy,
                               msg->rastport, msg->dstx, msg->dsty,
                               msg->width, msg->height);
}

VOID METHOD(HiddVC4Gallium, Hidd_Gallium, DestroyPipeScreen)
{
    struct vc4galliumstaticdata *sd = SD(cl);
    struct vc4_v3d_state *v3d = &sd->v3d;

    /* The direct path builds the pipe_screen in mesa3dgl via
     * vc4_screen_create(), so the base class's no-op DestroyPipeScreen
     * would leak the Mesa screen, BO cache, and driver allocations.
     * Dispatch into the screen's own destroy fn here. */
    D(bug("[VC4Gallium] %s(screen=%p)\n", __PRETTY_FUNCTION__, msg->screen));

    if (msg->screen)
    {
        struct pipe_screen *screen = (struct pipe_screen *)msg->screen;

        /* Quiesce before the screen frees its BOs: context teardown can
         * flush a trailing submission, and nothing polls after this call.
         * Without the wait, screen->destroy GEM_CLOSEs memory the GPU is
         * still rendering from, and a pending FLDONE->CT1 handoff would
         * kick a stale RCL into the NEXT GL session's first frame. */
        vc4_aros_wait_idle(sd);
        vc4_aros_dma_wait_idle(sd);

        screen->destroy(screen);
    }

    /* Drop the winsys bookkeeping for this screen. TRUE means it was the
     * last one, so the hardware reset below is safe; with a second screen
     * still live (a capability-probe context, or a fullscreen toggle
     * building the new context before dropping the old) it would free the
     * survivor's BOs and clear V3D state under its rendering. */
    if (!aros_drm_release_bridge())
        return;

    /* Return the driver to a clean, idle, empty state for the next GL
     * session: sweep whatever Mesa leaked (plus the async-DMA pin and the
     * frame pools) and drop the per-session submission/scanout state. */
    vc4_aros_release_all_bos(sd);

    if (v3d->v3d_available)
    {
        v3d->pending_render = FALSE;
        v3d->pending_ct1ca = 0;
        v3d->pending_ct1ea = 0;
        v3d->overflow_bus = 0;
        v3d->overflow_size = 0;
        v3d->overflow_handed = 0;
        /* GPU is idle now — W1C any leftover event latches so the next
         * session's first poll can't act on this session's events. */
        V3D_WRITE(v3d, V3D_INTCTL,
                  V3D_INT_FLDONE | V3D_INT_FRDONE | V3D_INT_OUTOMEM);
    }

    ObtainSemaphore(&sd->bo_lock);
    sd->scanout_phys[0] = 0;
    sd->scanout_phys[1] = 0;
    sd->scanout_size = 0;
    ReleaseSemaphore(&sd->bo_lock);
}

/*
 * Wait for the V3D to finish all submitted work, then flush its L2
 * cache so the rendered pixels are visible to subsequent DMA/CPU
 * reads of the source BO.
 *
 * Called from the bridge's wait_idle entry (used by glReadPixels and
 * any other CPU-read of a BO), and once internally from
 * vc4_aros_display_blit before kicking the DMA.
 */
void vc4_aros_wait_idle(struct vc4galliumstaticdata *sd)
{
    struct vc4_v3d_state *v3d = &sd->v3d;
#if VC4G_PROFILE
    ULONG _w_t0 = VC4G_NOW_US();
#endif
    /* Serialize V3D access against submit_cl / v3d_wait_seqno (e.g. the
     * display task waiting here while a render task is in submit_cl). */
    ObtainSemaphore(&sd->render_lock);
    /* Tiered wait (see VC4_GPUWAIT_* in vc4gallium_intern.h): tight spin for
     * µs-precise completion of short jobs, then ~1 ms timer naps so input
     * keeps running while the GPU renders. (Blocking on the vblank signal
     * from the start quantized every wait to ~20 ms and phase-locked the
     * zero-copy GL frame time to 40 ms; pure spinning starves the mouse.)
     * Beyond the nap window, fall back to CPU-free vblank blocking. */
    BYTE wsig = vc4_wait_enter(sd);
    ULONG ticks = 0;
    /* Both paths bound the wait to roughly the same wall clock: vblank
     * ticks are ~20 ms, the signal-less fallback paces itself at
     * VC4_GPUWAIT_NAP_US. The fallback used to poll flat out with a
     * 3.5M budget — seconds of spinning while holding render_lock, which
     * is what a stalled frame looked like from the outside. */
    ULONG budget = (wsig >= 0) ? 60 : (60 * 20000 / VC4_GPUWAIT_NAP_US);
    ULONG spin_start = gallium_now_us();

    while (v3d->finished_seqno < v3d->seqno && ticks < budget)
    {
        vc4_v3d_advance_counters(v3d);
        if (v3d->finished_seqno >= v3d->seqno)
            break;

        /* Poll-driven interrupt servicing: reads + W1C-clears INTCTL and does
         * the FLDONE -> CT1 handoff and OUTOMEM handling. The V3D IRQ is masked
         * (see vc4_v3d_init), so this poll is the only thing advancing the
         * pipeline. */
        vc4_v3d_service_interrupts(v3d);

        /* OUTOMEM (latch and PCS.BMOOM level) is handled inside
         * vc4_v3d_service_interrupts — a blind W1C here would eat the
         * edge-triggered latch before the overspill feed runs. */

        {
            ULONG waited = gallium_now_us() - spin_start;
            if (waited < VC4_GPUWAIT_SPIN_US)
                continue;
            if (waited < VC4_GPUWAIT_NAP_WINDOW)
            {
                vc4_gpu_nap(sd, VC4_GPUWAIT_NAP_US);
                continue;
            }

            /* Live hang probe: state DURING the stall, not post-reset.
             * Normal frames never wait this long, so these only fire on
             * genuine hangs. */
            {
                static ULONG midlog = 0;
                if (midlog < 6)
                {
                    midlog++;
                    bug("[VC4Gallium] wait_idle probe @%dms: want=%d fin=%d "
                        "CT1CS=0x%08x CT1CA=0x%08x BFC=%d RFC=%d "
                        "pend=%d kicks=%d\n",
                        waited / 1000, v3d->seqno, v3d->finished_seqno,
                        V3D_READ(v3d, V3D_CT1CS), V3D_READ(v3d, V3D_CT1CA),
                        V3D_READ(v3d, V3D_BFC) & 0xff,
                        V3D_READ(v3d, V3D_RFC) & 0xff,
                        (LONG)v3d->pending_render, v3d->kick_count);
                }
            }
        }

        ticks++;
        /* Block until the next vblank (~20 ms, CPU-free) on the registered
         * path. With no free signal (the task has none left to allocate)
         * blocking is impossible, so pace the poll instead of spinning. */
        if (wsig >= 0)
            Wait(1UL << wsig);
        else
            vc4_gpu_nap(sd, VC4_GPUWAIT_NAP_US);
    }

    vc4_wait_leave(sd, wsig);

    /* Timeout recovery: reset the control lists (CTRSTA) and advance
     * finished_seqno so the next submission isn't perpetually trying to
     * "catch up" to a render that will never complete. */
    if (v3d->finished_seqno < v3d->seqno)
    {
        ULONG ct0_before = V3D_READ(v3d, V3D_CT0CS);
        ULONG ct1_before = V3D_READ(v3d, V3D_CT1CS);
        ULONG ct0_after, ct1_after;

        bug("[VC4Gallium] wait_idle TIMEOUT: want seqno=%d fin=%d "
            "CT0CS=0x%08x CT1CS=0x%08x — writing CTRSTA\n",
            v3d->seqno, v3d->finished_seqno, ct0_before, ct1_before);
        bug("[VC4Gallium]   PCS=0x%08x CT0CA=0x%08x CT0EA=0x%08x "
            "CT1CA=0x%08x CT1EA=0x%08x pending=%d\n",
            V3D_READ(v3d, V3D_PCS),
            V3D_READ(v3d, V3D_CT0CA), V3D_READ(v3d, V3D_CT0EA),
            V3D_READ(v3d, V3D_CT1CA), V3D_READ(v3d, V3D_CT1EA),
            (LONG)v3d->pending_render);
        bug("[VC4Gallium]   BFC=%d RFC=%d INTCTL=0x%08x ERRSTAT=0x%08x "
            "SRQCS=0x%08x BPCA=0x%08x BPCS=0x%08x BPOA=0x%08x handed=%d\n",
            V3D_READ(v3d, V3D_BFC) & 0xff, V3D_READ(v3d, V3D_RFC) & 0xff,
            V3D_READ(v3d, V3D_INTCTL), V3D_READ(v3d, V3D_ERRSTAT),
            V3D_READ(v3d, V3D_SRQCS),
            V3D_READ(v3d, V3D_BPCA), V3D_READ(v3d, V3D_BPCS),
            V3D_READ(v3d, V3D_BPOA), v3d->overflow_handed);
        bug("[VC4Gallium]   kicks=%d fl=%d fr=%d pend_ca=0x%08x "
            "pend_ea=0x%08x\n",
            v3d->kick_count, v3d->int_fldone, v3d->int_frdone,
            v3d->pending_ct1ca, v3d->pending_ct1ea);
        v3d_dump_last_submit();

        V3D_WRITE(v3d, V3D_CT0CS, V3D_CTCS_CTRSTA);
        V3D_WRITE(v3d, V3D_CT1CS, V3D_CTCS_CTRSTA);

        ct0_after = V3D_READ(v3d, V3D_CT0CS);
        ct1_after = V3D_READ(v3d, V3D_CT1CS);
        bug("[VC4Gallium] wait_idle TIMEOUT: post-CTRSTA CT0CS=0x%08x "
            "CT1CS=0x%08x — CT1 %s reset\n",
            ct0_after, ct1_after,
            (ct1_after & V3D_CTCS_CTRUN) ? "DID NOT" : "appears");

        /* All in-flight jobs died with the reset — drop any pending CT1
         * handoff so a later poll can't kick a dead job's RCL. */
        v3d->pending_render = FALSE;
        v3d->finished_seqno = v3d->seqno;
        v3d->last_bfc = V3D_READ(v3d, V3D_BFC) & 0xff;
        v3d->last_rfc = V3D_READ(v3d, V3D_RFC) & 0xff;
        v3d->bfc_completed = v3d->seqno;
        v3d->rfc_completed = v3d->seqno;
    }

    /* Flush V3D L2 cache — ensures GPU writes are in main memory */
    V3D_WRITE(v3d, V3D_L2CACTL, V3D_L2CACTL_L2CCLR);

    ReleaseSemaphore(&sd->render_lock);
#if VC4G_PROFILE
    {
        static ULONG _w_acc, _w_calls, _w_max;
        ULONG _w_d = VC4G_NOW_US() - _w_t0;
        _w_acc += _w_d;
        _w_calls++;
        if (_w_d > _w_max)
            _w_max = _w_d;
        if (_w_calls >= 120)
        {
            VC4G_PROF("[VC4Prof] wait_idle: %lu calls, avg=%lu us, max=%lu us "
                      "(per ~120 wait_idle calls)\n",
                _w_calls, _w_acc / _w_calls, _w_max);
            _w_acc = 0; _w_calls = 0; _w_max = 0;
        }
    }
#endif
}

/*
 * Scanout page query for zero-copy fullscreen GL. Resolves the vc4gfx
 * framebuffer's two flip pages and announces them (via sd->scanout_*)
 * as acceptable GEM_OPEN names, so the Mesa side can wrap them as BOs
 * and render straight into the back page. Returns 0 on success, -1
 * when the bitmap isn't a flippable vc4gfx framebuffer.
 */
int vc4_aros_get_scanout(struct vc4galliumstaticdata *sd, OOP_Object *bm_obj,
                         struct vc4_aros_scanout *out)
{
    IPTR front = 0, back = 0, pitch = 0, width = 0, height = 0;

    if (!bm_obj || !sd->hiddVC4GfxBMAB || !sd->hiddBitMapAB)
        return -1;

    OOP_GetAttr(bm_obj,
        sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_Drawable, &front);
    OOP_GetAttr(bm_obj,
        sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_BackDrawable, &back);
    OOP_GetAttr(bm_obj, sd->hiddBitMapAB + aoHidd_BitMap_BytesPerRow, &pitch);
    OOP_GetAttr(bm_obj, sd->hiddBitMapAB + aoHidd_BitMap_Width, &width);
    OOP_GetAttr(bm_obj, sd->hiddBitMapAB + aoHidd_BitMap_Height, &height);

    if (!front || !back || !pitch || !width || !height)
        return -1;

    /* Stable page identity regardless of which one is currently front. */
    out->name[0] = (front < back) ? (ULONG)front : (ULONG)back;
    out->name[1] = (front < back) ? (ULONG)back : (ULONG)front;
    out->back    = ((ULONG)back == out->name[1]) ? 1 : 0;
    out->pitch   = (ULONG)pitch;
    out->width   = (ULONG)width;
    out->height  = (ULONG)height;

    /* Publish for GEM_OPEN. */
    ObtainSemaphore(&sd->bo_lock);
    sd->scanout_phys[0] = out->name[0];
    sd->scanout_phys[1] = out->name[1];
    sd->scanout_size = (ULONG)pitch * (ULONG)height;
    ReleaseSemaphore(&sd->bo_lock);

    return 0;
}

/*
 * Flip the vc4gfx framebuffer pages after the GPU has rendered a full
 * frame into the back page. Waits for the GPU (and any in-flight blit
 * DMA) before the page becomes visible. Returns 0 on success.
 */
int vc4_aros_flip_scanout(struct vc4galliumstaticdata *sd, OOP_Object *bm_obj)
{
    struct TagItem fliptags[] =
    {
        { 0, TRUE },
        { TAG_DONE, 0 }
    };
    IPTR back_before = 0, back_after = 0;

    if (!bm_obj || !sd->hiddVC4GfxBMAB)
        return -1;

    OOP_GetAttr(bm_obj,
        sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_BackDrawable, &back_before);
    if (!back_before)
        return -1;

    vc4_aros_wait_idle(sd);
    vc4_aros_dma_wait_idle(sd);

    /* The entry frame's blit pinned its source BO until "the next blit";
     * on the flip path there is no next blit, so release it here now
     * that the DMA engine is idle. */
    ObtainSemaphore(&sd->bo_lock);
    if (sd->dma_pinned_handle)
    {
        vc4_aros_bo_unref_locked(sd, sd->dma_pinned_handle);
        sd->dma_pinned_handle = 0;
    }
    ReleaseSemaphore(&sd->bo_lock);

    fliptags[0].ti_Tag = sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_Flip;
    OOP_SetAttrs(bm_obj, fliptags);

    OOP_GetAttr(bm_obj,
        sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_BackDrawable, &back_after);

    /* The flip succeeded iff the back page changed. */
    return (back_after && back_after != back_before) ? 0 : -1;
}

/*
 * Zero-copy windowed present: show a rendered BO as an HVS overlay
 * plane at (x,y) on the screen's framebuffer bitmap. The BO stays
 * pinned while on scanout; each call retargets the plane and unpins
 * the previous buffer (the hidd's overlay path waits for the vblank
 * latch before returning, so the old buffer is off-screen by then).
 * Returns 0 on success, -1 when the overlay is unavailable (firmware
 * owns the display / scaled desktop / bad handle) — caller blits.
 */
int vc4_aros_set_overlay(struct vc4galliumstaticdata *sd,
                         OOP_Object *bm_obj,
                         ULONG src_bo_handle, ULONG src_stride,
                         LONG x, LONG y, ULONG w, ULONG h,
                         ULONG dest_w, ULONG dest_h)
{
    struct vc4gfx_overlay desc;
    struct TagItem ovltags[] =
    {
        { 0, (IPTR)&desc },
        { TAG_DONE, 0 }
    };
    IPTR active = 0;
    ULONG prev;

    if (!bm_obj || !sd->hiddVC4GfxBMAB || !w || !h)
        return -1;

    ObtainSemaphore(&sd->bo_lock);
    if (src_bo_handle >= VC4_MAX_BOS || !sd->bo_table[src_bo_handle].vaddr
        || (ULONG)src_stride * h > sd->bo_table[src_bo_handle].size)
    {
        ReleaseSemaphore(&sd->bo_lock);
        return -1;
    }
    desc.ovl_Phys   = sd->bo_table[src_bo_handle].bus_addr & 0x3fffffff;
    desc.ovl_Pitch  = src_stride;
    desc.ovl_Width  = w;
    desc.ovl_Height = h;
    desc.ovl_X      = x;
    desc.ovl_Y      = y;
    desc.ovl_DestW  = dest_w;
    desc.ovl_DestH  = dest_h;

    /* Pin the new buffer before showing it; the previous pin is
     * released only after the hidd confirms the switch latched. */
    sd->bo_table[src_bo_handle].refcount++;
    prev = sd->overlay_pinned_handle;
    sd->overlay_pinned_handle = src_bo_handle;
    sd->overlay_bm = bm_obj;
    ReleaseSemaphore(&sd->bo_lock);

    /* The frame must be complete before it becomes visible — but that is
     * the CALLER's contract now (bridge v5): mesa3dgl waits on the seqno
     * of the frame it displays (normally the previous frame, already
     * finished) before calling here. Waiting for full GPU idle at this
     * point serialized CPU and GPU (frame time = CPU + GPU), costing tens
     * of ms per frame in a GPU-bound scene. */

    ovltags[0].ti_Tag = sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_Overlay;
    OOP_SetAttrs(bm_obj, ovltags);
    OOP_GetAttr(bm_obj,
        sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_Overlay, &active);

    {
        static ULONG _ovl_n = 0;

        _ovl_n++;
        if (_ovl_n <= 4 || (_ovl_n & 255) == 0)
            bug("[VC4Gallium] set_overlay #%u: bo=%lu %lux%lu at %d,%d "
                "active=%d\n", _ovl_n, (unsigned long)src_bo_handle,
                (unsigned long)w, (unsigned long)h, x, y, (LONG)active);
    }

    ObtainSemaphore(&sd->bo_lock);
    if (!active)
    {
        /* Rejected: nothing is on scanout — drop both pins. */
        vc4_aros_bo_unref_locked(sd, src_bo_handle);
        if (prev)
            vc4_aros_bo_unref_locked(sd, prev);
        sd->overlay_pinned_handle = 0;
        sd->overlay_bm = NULL;
        ReleaseSemaphore(&sd->bo_lock);
        return -1;
    }
    /* Release the pin the previous present took — including when the same
     * BO is shown twice in a row: we added a fresh pin above, so skipping
     * this would strand one reference per repeat and the BO could never be
     * freed. */
    if (prev)
        vc4_aros_bo_unref_locked(sd, prev);
    ReleaseSemaphore(&sd->bo_lock);
    return 0;
}

void vc4_aros_clear_overlay(struct vc4galliumstaticdata *sd,
                            OOP_Object *bm_obj)
{
    struct TagItem ovltags[] =
    {
        { 0, 0 },       /* NULL descriptor = remove */
        { TAG_DONE, 0 }
    };
    OOP_Object *target = bm_obj ? bm_obj : sd->overlay_bm;

    if (target && sd->hiddVC4GfxBMAB)
    {
        ovltags[0].ti_Tag = sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_Overlay;
        OOP_SetAttrs(target, ovltags);
    }

    ObtainSemaphore(&sd->bo_lock);
    if (sd->overlay_pinned_handle)
    {
        vc4_aros_bo_unref_locked(sd, sd->overlay_pinned_handle);
        sd->overlay_pinned_handle = 0;
    }
    sd->overlay_bm = NULL;
    ReleaseSemaphore(&sd->bo_lock);
}

/*
 * Blit a rectangle from a BO to an AROS bitmap. Waits for GPU work
 * to finish first; pins/unpins the BO around the access; tries DMA
 * to the framebuffer phys address when the dest is a vc4gfx bitmap,
 * falls back to WritePixelArray otherwise.
 *
 * Caller supplies resolved fields — no Mesa types cross the API.
 */
void vc4_aros_display_blit(struct vc4galliumstaticdata *sd,
                           ULONG src_bo_handle,
                           ULONG src_stride,
                           ULONG src_offset,
                           ULONG cpp,
                           OOP_Object *bm_obj,
                           LONG dst_x, LONG dst_y,
                           ULONG width, ULONG height)
{
    UBYTE *src_addr;
    ULONG width_bytes = width * cpp;
    ULONG bo_size;
    BOOL _used_dma = FALSE;
    ULONG _t_entry = VC4G_NOW_US();
    ULONG _t_after_validate, _t_after_v3d, _t_after_blit;

    /* Validate handle and pin BO under bo_lock so a concurrent gem_close
     * can't free it out from under us. */
    ObtainSemaphore(&sd->bo_lock);
    if (src_bo_handle == 0 || src_bo_handle >= VC4_MAX_BOS ||
        sd->bo_table[src_bo_handle].refcount == 0)
    {
        ReleaseSemaphore(&sd->bo_lock);
        bug("[VC4Gallium] display_blit: invalid bo handle %d "
            "(max=%d, refcount=%d)\n",
            src_bo_handle, VC4_MAX_BOS,
            (src_bo_handle < VC4_MAX_BOS)
                ? sd->bo_table[src_bo_handle].refcount : 0);
        return;
    }

    /* Bounds-check the source rectangle against the BO size. Bridge is
     * an ABI boundary, so don't trust offset/stride/cpp/width/height —
     * a malformed combination would otherwise DMA past the BO end. */
    bo_size = sd->bo_table[src_bo_handle].size;
    if (width == 0 || height == 0 || cpp == 0 ||
        width_bytes / cpp != width ||
        width_bytes > src_stride ||
        src_offset > bo_size ||
        (ULONG)(height - 1) > (bo_size - src_offset) / src_stride ||
        (ULONG)(height - 1) * src_stride + width_bytes > bo_size - src_offset)
    {
        ReleaseSemaphore(&sd->bo_lock);
        bug("[VC4Gallium] display_blit: rect out of range bo=%d size=%d "
            "offset=%d stride=%d cpp=%d %dx%d\n",
            src_bo_handle, bo_size, src_offset, src_stride, cpp,
            width, height);
        return;
    }

    src_addr = (UBYTE *)sd->bo_table[src_bo_handle].vaddr + src_offset;
    sd->bo_table[src_bo_handle].refcount++;
    ReleaseSemaphore(&sd->bo_lock);

    D(bug("[VC4Gallium] display_blit: bo=%d %dx%d cpp=%d stride=%d "
          "src=0x%08x -> bm=%p at %d,%d\n",
        src_bo_handle, width, height, cpp, src_stride,
        (ULONG)(IPTR)src_addr, bm_obj, dst_x, dst_y));

    /* Unconditional, rate-limited: the geometry-gated logs below go
     * silent when a later app reuses an earlier app's geometry, which
     * makes "is present happening at all" undiagnosable from serial. */
    {
        static ULONG _present_n = 0;

        _present_n++;
        if (_present_n <= 8 || (_present_n & 255) == 0)
            bug("[VC4Gallium] present blit #%u: %dx%d at %d,%d bm=%p\n",
                _present_n, width, height, dst_x, dst_y, bm_obj);
    }

    _t_after_validate = VC4G_NOW_US();

    /* GPU must be idle before we read the BO. */
    vc4_aros_wait_idle(sd);

    D({
        volatile ULONG *p = (volatile ULONG *)src_addr;
        ULONG row_words = src_stride / 4;
        CacheClearE(src_addr, src_stride * height, CACRF_ClearD);
        bug("[VC4Gallium] BO sniff: [0]=0x%08x [middle]=0x%08x [last]=0x%08x\n",
            p[0],
            p[(height / 2) * row_words + (width / 2)],
            p[(height - 1) * row_words + (width - 1)]);
    });

    _t_after_v3d = VC4G_NOW_US();

    /* Fast path: DMA into a vc4gfx framebuffer phys address. */
    {
        IPTR fb_addr = 0;
        IPTR fb_bpr = 0;

        if (bm_obj && sd->hiddVC4GfxBMAB)
        {
            OOP_GetAttr(bm_obj,
                sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_Drawable,
                &fb_addr);
            OOP_GetAttr(bm_obj,
                sd->hiddBitMapAB + aoHidd_BitMap_BytesPerRow,
                &fb_bpr);
        }

        if (fb_addr && fb_bpr)
        {
            BOOL flip = FALSE;

            /* Full-frame blit with a flippable framebuffer: blit into
             * the back page and flip — tear-free, and the DMA never
             * writes to live scanout. */
            if (dst_x == 0 && dst_y == 0)
            {
                IPTR back = 0, bm_w = 0, bm_h = 0;

                OOP_GetAttr(bm_obj,
                    sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_BackDrawable,
                    &back);
                if (back)
                {
                    OOP_GetAttr(bm_obj,
                        sd->hiddBitMapAB + aoHidd_BitMap_Width, &bm_w);
                    OOP_GetAttr(bm_obj,
                        sd->hiddBitMapAB + aoHidd_BitMap_Height, &bm_h);
                    if (width == (ULONG)bm_w && height == (ULONG)bm_h)
                    {
                        fb_addr = back;
                        flip = TRUE;
                    }
                }
            }

            {
                /* Log on geometry change; ignore fb_addr (it alternates
                 * between the two flip pages every frame). */
                static struct {
                    ULONG w, h, bpr;
                    LONG x, y, flip;
                    ULONG count;
                } dlast = { 0, 0, 0, -1, -1, -1, 0 };

                if (dlast.count < 100 &&
                    (width != dlast.w || height != dlast.h ||
                     (ULONG)fb_bpr != dlast.bpr || dst_x != dlast.x ||
                     dst_y != dlast.y || (LONG)flip != dlast.flip))
                {
                    dlast.count++;
                    dlast.w = width; dlast.h = height;
                    dlast.bpr = (ULONG)fb_bpr;
                    dlast.x = dst_x; dlast.y = dst_y;
                    dlast.flip = flip;
                    bug("[VC4Gallium] display_blit: DMA %dx%d cpp=%d to "
                        "fb=0x%08x bpr=%d at %d,%d flip=%d\n",
                        width, height, cpp, (ULONG)fb_addr, (ULONG)fb_bpr,
                        dst_x, dst_y, (LONG)flip);
                }
            }

            _used_dma = gallium_dma_blit_direct(sd,
                (ULONG)(IPTR)src_addr, src_stride,
                (ULONG)fb_addr + dst_y * (ULONG)fb_bpr + dst_x * cpp,
                (ULONG)fb_bpr,
                width_bytes, height);

            /* On DMA failure (no TDMODE channel, CB alloc failure) fall
             * through to the CPU path below, and skip the flip — the back
             * page never received the frame. */
            if (_used_dma && flip)
            {
                /* The blit runs asynchronously — the page must be
                 * complete before it becomes visible. */
                struct TagItem fliptags[] =
                {
                    { sd->hiddVC4GfxBMAB + aoHidd_VideoCoreGfxBitMap_Flip, TRUE },
                    { TAG_DONE, 0 }
                };

                vc4_aros_dma_wait_idle(sd);
                OOP_SetAttrs(bm_obj, fliptags);
            }
        }

        if (!_used_dma)
        {
            static struct {
                ULONG w, h;
                LONG x, y;
                APTR bm;
                ULONG count;
            } clast = { 0, 0, -1, -1, NULL, 0 };

            if (clast.count < 100 &&
                (width != clast.w || height != clast.h ||
                 dst_x != clast.x || dst_y != clast.y || (APTR)bm_obj != clast.bm))
            {
                clast.count++;
                clast.w = width; clast.h = height;
                clast.x = dst_x; clast.y = dst_y;
                clast.bm = (APTR)bm_obj;
                bug("[VC4Gallium] display_blit: CPU path (no fb addr / DMA failed) "
                    "%dx%d at %d,%d bm=%p\n",
                    width, height, dst_x, dst_y, bm_obj);
            }

            /* Fallback for non-vc4gfx bitmaps — CPU reads BO data,
             * so we must flush ARM D-cache to see fresh GPU output. */
            CacheClearE(src_addr, src_stride * height, CACRF_ClearD);

            struct RastPort *rp = CreateRastPort();
            if (rp)
            {
                /* WritePixelArray needs a struct BitMap *, not the OOP
                 * object. Resolve via aHidd_BitMap_BMStruct. */
                IPTR struct_bm = 0;
                OOP_GetAttr(bm_obj,
                    sd->hiddBitMapAB + aoHidd_BitMap_BMStruct,
                    &struct_bm);
                if (struct_bm)
                {
                    rp->BitMap = (struct BitMap *)struct_bm;
                    WritePixelArray(src_addr, 0, 0, src_stride,
                                    rp, dst_x, dst_y,
                                    width, height, AROS_PIXFMT);
                }
                FreeRastPort(rp);
            }
        }
    }

    _t_after_blit = VC4G_NOW_US();

    /* Drop the pin. DMA path: the previous frame's DMA already finished
     * inside gallium_dma_blit_direct, so release its pin and hand our own to
     * the new DMA — keeps the BO alive even if Mesa runs GEM_CLOSE before the
     * next swap. CPU path: BO consumed synchronously, release immediately. */
    ObtainSemaphore(&sd->bo_lock);
    if (_used_dma)
    {
        ULONG prev = sd->dma_pinned_handle;
        sd->dma_pinned_handle = src_bo_handle;
        if (prev)
            vc4_aros_bo_unref_locked(sd, prev);
    }
    else
    {
        vc4_aros_bo_unref_locked(sd, src_bo_handle);
    }
    ReleaseSemaphore(&sd->bo_lock);

    VC4G_PROFF("[VC4Prof] display_blit: validate=%ld v3d_wait=%ld blit=%ld total=%ld path=%s %ldx%ld\n",
        _t_after_validate - _t_entry,
        _t_after_v3d - _t_after_validate,
        _t_after_blit - _t_after_v3d,
        _t_after_blit - _t_entry,
        _used_dma ? "DMA" : "CPU",
        width, height);
}
