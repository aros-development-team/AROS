/*
    Copyright 2025, The AROS Development Team. All rights reserved.

    VC4 Gallium 3D HIDD - Module initialization
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <hidd/gallium.h>
#include <hidd/gfx.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/mbox.h>
#include <proto/dma.h>
#include <proto/kernel.h>

#include <hardware/videocore.h>

#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>

/* Peripheral base address — initialized at module load */
IPTR __arm_periiobase __attribute__((used)) = 0;

/* Module-global kernel.resource base — referenced implicitly by the
 * Krn* macros from <proto/kernel.h>. */
APTR KernelBase __attribute__((used)) = NULL;

/* dma.resource base for channel allocation. */
APTR DMABase = NULL;

#include "vc4gallium_intern.h"
#include "vc4_drm_aros.h"
#include "vc4_v3d.h"

/* IID for vc4gfx bitmap — matches vc4gfx_bitmap.h */
#define IID_Hidd_BitMap_VideoCore4  "hidd.bitmap.bcmvc4"

/*
 * INTB_VERTB server: runs each vblank (~50 Hz, the BCM2708 scheduling
 * heartbeat). It only signals the one registered GPU waiter — a lockless read
 * plus Signal(), nothing that can block or deadlock against the heartbeat IRQ.
 */
AROS_INTH1(vc4_vblank_server, struct vc4galliumstaticdata *, sd)
{
    AROS_INTFUNC_INIT

    struct Task *t = sd->vblank_waiter;
    if (t)
        Signal(t, 1UL << sd->vblank_waiter_sig);

    return 0;   /* let other VBlank servers run */

    AROS_INTFUNC_EXIT
}

/*
 * Claim the single vblank-waiter slot for the calling task. Serialized by
 * wait_gate (a second waiter blocks here, CPU-free, until the first leaves).
 * Returns the allocated signal bit, or -1 if none free (caller then falls
 * back to a bounded busy-poll). The gate is held until vc4_wait_leave().
 */
BYTE vc4_wait_enter(struct vc4galliumstaticdata *sd)
{
    BYTE sig;

    ObtainSemaphore(&sd->wait_gate);

    sig = AllocSignal(-1);
    if (sig >= 0)
    {
        sd->vblank_waiter_sig = sig;
        /* Publish the sig before the task pointer the IRQ server tests. */
        __asm__ __volatile__("dmb sy" ::: "memory");
        sd->vblank_waiter = FindTask(NULL);
    }

    return sig;
}

void vc4_wait_leave(struct vc4galliumstaticdata *sd, BYTE sig)
{
    if (sig >= 0)
    {
        sd->vblank_waiter = NULL;
        __asm__ __volatile__("dmb sy" ::: "memory");
        FreeSignal(sig);
    }
    ReleaseSemaphore(&sd->wait_gate);
}

/*
 * Diagnostic: log the current ARM/core/V3D firmware clocks. InitLib forces
 * V3D to max only once at boot; if the firmware governor later re-idle-clocks
 * it behind our back, this (called from CreatePipeScreen, i.e. every GL
 * context start) makes that visible in the serial log.
 */
void vc4_log_clocks(struct vc4galliumstaticdata *sd, const char *when)
{
#define MBoxBase sd->MBoxBase
#define VCMB_PROPCHAN 8
    D({
    static const ULONG clkid[3] = { 3 /* arm */, 4 /* core */, 5 /* v3d */ };
    volatile ULONG *msg = sd->mbox_msg;
    ULONG rate[3];
    int i;

    ObtainSemaphore(&sd->mbox_lock);
    for (i = 0; i < 3; i++)
    {
        msg[0] = AROS_LE2LONG(8 * 4);
        msg[1] = AROS_LE2LONG(VCTAG_REQ);
        msg[2] = AROS_LE2LONG(VCTAG_GETCLKRATE);
        msg[3] = AROS_LE2LONG(8);
        msg[4] = AROS_LE2LONG(4);
        msg[5] = AROS_LE2LONG(clkid[i]);
        msg[6] = 0;
        msg[7] = 0;
        MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);
        rate[i] = AROS_LE2LONG(msg[6]);
    }
    ReleaseSemaphore(&sd->mbox_lock);

    bug("[VC4Gallium] clocks (%s): arm=%u core=%u v3d=%u Hz\n",
        when, rate[0], rate[1], rate[2]);
    });
#undef VCMB_PROPCHAN
#undef MBoxBase
}

static int HiddVC4Gallium_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[VC4Gallium] ExpungeLib\n"));

    /* Stop the vblank server before tearing anything down, so it can't fire
     * into freed state. */
    if (LIBBASE->sd.vblank_added)
    {
        RemIntServer(INTB_VERTB, &LIBBASE->sd.vblank_server);
        LIBBASE->sd.vblank_added = FALSE;
    }

    /* Quiesce V3D interrupts and unhook our handler before tearing down
     * the rest of the module, otherwise a late OUTOMEM IRQ would call into
     * freed memory. */
    if (LIBBASE->sd.v3d.v3d_available)
    {
        V3D_WRITE(&LIBBASE->sd.v3d, V3D_INTDIS,
                  V3D_INT_FLDONE | V3D_INT_FRDONE | V3D_INT_OUTOMEM);
        V3D_WRITE(&LIBBASE->sd.v3d, V3D_INTCTL,
                  V3D_INT_FLDONE | V3D_INT_FRDONE | V3D_INT_OUTOMEM);
    }
    if (LIBBASE->sd.v3d.irq_handle)
    {
        KrnRemIRQHandler(LIBBASE->sd.v3d.irq_handle);
        LIBBASE->sd.v3d.irq_handle = NULL;
    }

    /* Release any GPU memory still locked by Mesa or our pools, so the
     * firmware can reclaim it instead of holding it until reboot. */
    vc4_aros_release_all_bos(&LIBBASE->sd);

    if (LIBBASE->sd.gpu_timer_ok)
    {
        CloseDevice((struct IORequest *)&LIBBASE->sd.gpu_timer_template);
        LIBBASE->sd.gpu_timer_ok = FALSE;
    }

    if (LIBBASE->sd.dmaStaging)
        FreeMem(LIBBASE->sd.dmaStaging, LIBBASE->sd.dmaStagingSize);
    if (LIBBASE->sd.dmaCBRaw)
        FreeMem(LIBBASE->sd.dmaCBRaw, LIBBASE->sd.dmaCBRawSize);
    if ((DMABase) && (LIBBASE->sd.dmaChannel >= 0))
    {
        DMAFreeChannel(LIBBASE->sd.dmaChannel);
        LIBBASE->sd.dmaChannel = -1;
    }

    if (LIBBASE->sd.mbox_msg_raw)
    {
        FreeMem(LIBBASE->sd.mbox_msg_raw, 256 + (MBOX_MSG_ALIGN - 1));
        LIBBASE->sd.mbox_msg_raw = NULL;
        LIBBASE->sd.mbox_msg = NULL;
    }

    if (LIBBASE->sd.UtilityBase)
        CloseLibrary(LIBBASE->sd.UtilityBase);

    if (LIBBASE->sd.hiddBitMapAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_BitMap);

    if (LIBBASE->sd.hiddVC4GfxBMAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_BitMap_VideoCore4);

    if (LIBBASE->sd.hiddGalliumAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    return TRUE;
}

static int HiddVC4Gallium_InitLib(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[VC4Gallium] InitLib\n"));

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);
    D(bug("[VC4Gallium] Peripheral base: 0x%08lx\n", __arm_periiobase));

    LIBBASE->sd.MBoxBase = OpenResource("mbox.resource");
    if (!LIBBASE->sd.MBoxBase)
    {
        bug("[VC4Gallium] Failed to open mbox.resource\n");
        return FALSE;
    }

    if (!(LIBBASE->sd.UtilityBase = OpenLibrary((STRPTR)"utility.library", 0)))
    {
        bug("[VC4Gallium] Failed to open utility.library\n");
        return FALSE;
    }

    if (!(LIBBASE->sd.hiddGalliumAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium)))
    {
        bug("[VC4Gallium] Failed to obtain Gallium attr base\n");
        CloseLibrary(LIBBASE->sd.UtilityBase);
        return FALSE;
    }

    /* Own our cache lines; see <proto/mbox.h>. */
    LIBBASE->sd.mbox_msg_raw = AllocMem(256 + (MBOX_MSG_ALIGN - 1), MEMF_PUBLIC | MEMF_CLEAR);
    if (!LIBBASE->sd.mbox_msg_raw)
    {
        bug("[VC4Gallium] Failed to alloc mailbox buffer\n");
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);
        CloseLibrary(LIBBASE->sd.UtilityBase);
        return FALSE;
    }
    LIBBASE->sd.mbox_msg = (volatile ULONG *)(((IPTR)LIBBASE->sd.mbox_msg_raw + (MBOX_MSG_ALIGN - 1)) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    InitSemaphore(&LIBBASE->sd.bo_lock);
    InitSemaphore(&LIBBASE->sd.mbox_lock);
    InitSemaphore(&LIBBASE->sd.wait_gate);
    InitSemaphore(&LIBBASE->sd.render_lock);

    /* Install the vblank int server that wakes GPU waiters (~50 Hz). */
    LIBBASE->sd.vblank_waiter = NULL;
    LIBBASE->sd.vblank_added = FALSE;
    LIBBASE->sd.vblank_server.is_Node.ln_Type = NT_INTERRUPT;
    LIBBASE->sd.vblank_server.is_Node.ln_Pri  = 0;
    LIBBASE->sd.vblank_server.is_Node.ln_Name = (STRPTR)"vc4gallium vblank";
    LIBBASE->sd.vblank_server.is_Code = (APTR)vc4_vblank_server;
    LIBBASE->sd.vblank_server.is_Data = &LIBBASE->sd;
    AddIntServer(INTB_VERTB, &LIBBASE->sd.vblank_server);
    LIBBASE->sd.vblank_added = TRUE;

    /* timer.device (UNIT_MICROHZ) for the GPU wait loops' microsleeps.
     * Only io_Device/io_Unit are kept — vc4_gpu_nap clones them into a
     * stack request per use. Non-fatal: without it the waits degrade to
     * busy-spinning inside the nap window. */
    LIBBASE->sd.gpu_timer_ok = FALSE;
    {
        struct timerequest *tt = &LIBBASE->sd.gpu_timer_template;

        tt->tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
        tt->tr_node.io_Message.mn_ReplyPort = NULL;
        tt->tr_node.io_Message.mn_Length = sizeof(*tt);
        if (OpenDevice("timer.device", UNIT_MICROHZ,
                       (struct IORequest *)tt, 0) == 0)
            LIBBASE->sd.gpu_timer_ok = TRUE;
        else
            bug("[VC4Gallium] timer.device unavailable — GPU waits will spin\n");
    }

    /* Initialize DMA state. The display blit needs a full DMA engine for
     * 2D stride mode; without one we fall back to the CPU blit path. */
    LIBBASE->sd.dmaChannel = -1;
    if ((DMABase = OpenResource("dma.resource")) != NULL)
        LIBBASE->sd.dmaChannel = DMAAllocChannel(DMACHF_TDMODE | DMACHF_IRQ);
    if (LIBBASE->sd.dmaChannel < 0)
        bug("[VC4Gallium] No 2D-capable DMA channel — using CPU display blit\n");
    LIBBASE->sd.dmaBusy = FALSE;
    LIBBASE->sd.dmaStaging = NULL;
    LIBBASE->sd.dmaStagingSize = 0;
    LIBBASE->sd.dmaCBRaw = NULL;
    LIBBASE->sd.dmaCBRawSize = 0;

    /* vc4gfx bitmap attr base is optional — without it we fall back to
     * WritePixelArray for display. Not fatal. */
    LIBBASE->sd.hiddVC4GfxBMAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_BitMap_VideoCore4);
    if (!LIBBASE->sd.hiddVC4GfxBMAB)
        D(bug("[VC4Gallium] vc4gfx bitmap attr base not available — will use CPU blit\n"));

    /* Standard BitMap attribute base for BytesPerRow etc. */
    LIBBASE->sd.hiddBitMapAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_BitMap);

    /* Power on V3D domain via mailbox.
     * BCM2835/2836 power domain ID 10 = V3D.
     * Without this, V3D registers read as zero. */
    {
#define VCPOWER_V3D         10
#define VCPOWER_STATE_ON    (1 << 0)
#define VCPOWER_STATE_WAIT  (1 << 1)
#define VCCLOCK_V3D         5       /* VideoCore clock id 5 = V3D */
#define VCMB_PROPCHAN       8
#ifdef MBoxBase
#undef MBoxBase
#endif
#define MBoxBase LIBBASE->sd.MBoxBase

        volatile ULONG *msg = LIBBASE->sd.mbox_msg;

        ObtainSemaphore(&LIBBASE->sd.mbox_lock);

        msg[0] = AROS_LE2LONG(8 * 4);
        msg[1] = AROS_LE2LONG(VCTAG_REQ);
        msg[2] = AROS_LE2LONG(VCTAG_SETPOWER);
        msg[3] = AROS_LE2LONG(8);              /* value buffer size */
        msg[4] = AROS_LE2LONG(8);              /* request size */
        msg[5] = AROS_LE2LONG(VCPOWER_V3D);
        msg[6] = AROS_LE2LONG(VCPOWER_STATE_ON | VCPOWER_STATE_WAIT);
        msg[7] = 0;

        MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);

        if (AROS_LE2LONG(msg[6]) & VCPOWER_STATE_ON)
            D(bug("[VC4Gallium] V3D power domain enabled\n"));
        else
            bug("[VC4Gallium] V3D power-on failed (response: 0x%08x)\n",
                AROS_LE2LONG(msg[6]));

        /* Query and force the V3D core clock. We drive V3D bare-metal, so the
         * firmware's on-demand governor never sees GPU load and idle-clocks
         * V3D low — throttling GPU-bound frames. Log current/max, then lock to
         * max. Logged unconditionally (once at init). */
        {
            ULONG v3d_cur, v3d_max;

            msg[0] = AROS_LE2LONG(8 * 4);
            msg[1] = AROS_LE2LONG(VCTAG_REQ);
            msg[2] = AROS_LE2LONG(VCTAG_GETCLKRATE);
            msg[3] = AROS_LE2LONG(8);
            msg[4] = AROS_LE2LONG(4);
            msg[5] = AROS_LE2LONG(VCCLOCK_V3D);
            msg[6] = 0;
            msg[7] = 0;
            MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);
            v3d_cur = AROS_LE2LONG(msg[6]);

            msg[0] = AROS_LE2LONG(8 * 4);
            msg[1] = AROS_LE2LONG(VCTAG_REQ);
            msg[2] = AROS_LE2LONG(VCTAG_GETCLKMAX);
            msg[3] = AROS_LE2LONG(8);
            msg[4] = AROS_LE2LONG(4);
            msg[5] = AROS_LE2LONG(VCCLOCK_V3D);
            msg[6] = 0;
            msg[7] = 0;
            MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);
            v3d_max = AROS_LE2LONG(msg[6]);

            bug("[VC4Gallium] V3D clock: current=%u Hz max=%u Hz\n",
                v3d_cur, v3d_max);

            if (v3d_max && v3d_cur < v3d_max)
            {
                msg[0] = AROS_LE2LONG(9 * 4);
                msg[1] = AROS_LE2LONG(VCTAG_REQ);
                msg[2] = AROS_LE2LONG(VCTAG_SETCLKRATE);
                msg[3] = AROS_LE2LONG(12);
                msg[4] = AROS_LE2LONG(12);
                msg[5] = AROS_LE2LONG(VCCLOCK_V3D);
                msg[6] = AROS_LE2LONG(v3d_max);
                msg[7] = 0;      /* skip_setting_turbo = 0 */
                msg[8] = 0;
                MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);
                bug("[VC4Gallium] V3D clock forced to %u Hz\n",
                    AROS_LE2LONG(msg[6]));
            }
        }

        /* The ARM core clock is raised to max in the bootstrap
         * (arch/arm-raspi/boot/boot.c:setup_arm_clock) — system-wide
         * concern, not the GPU driver's. */

        /* NOTE: do NOT arm V3D interrupts (see VC4_V3D_IRQ_ENA in
         * vc4_v3d.c) — and the ENABLE_QPU (0x00030012) handshake was
         * tried and does not stop the firmware from wedging its
         * mailbox task when the V3D line asserts. */

        ReleaseSemaphore(&LIBBASE->sd.mbox_lock);

#undef MBoxBase
#undef VCMB_PROPCHAN
    }

    if (!vc4_v3d_init(&LIBBASE->sd.v3d))
    {
        bug("[VC4Gallium] V3D initialization failed — "
            "GPU may be in use by firmware. Continuing without 3D.\n");
        /* Don't fail init — the module loads, but CreatePipeScreen
         * will return NULL if V3D isn't available. */
    }

    D(bug("[VC4Gallium] Module initialized successfully\n"));

    return TRUE;
}

ADD2INITLIB(HiddVC4Gallium_InitLib, 0)
ADD2EXPUNGELIB(HiddVC4Gallium_ExpungeLib, 0)

ADD2LIBS((STRPTR)"gallium.hidd", 7, static struct Library *, GalliumHiddBase);
