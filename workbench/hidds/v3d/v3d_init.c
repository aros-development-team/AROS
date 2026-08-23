/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    VideoCore VI (V3D) - driver initialisation and power-up.

    Waking the block takes three separate authorities, and skipping any
    one of them leaves every register reading the bus poison 0xdeadbeef:
    the firmware owns the clock (id 5, same as on the Pi 3), the PM block
    owns the power domain and the reset, and the 2711-only ASB instance
    next to V3D gates the bus bridges. The mailbox power tags are NOT
    among them - they answer "on" without any of this happening, which is
    how the first version of this sequence fooled itself.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/kernel.h>
#include <proto/mbox.h>

#include <hidd/gallium.h>
#include <hidd/gfx.h>

#include "v3d_intern.h"

#include LC_LIBDEFS_FILE

IPTR __arm_periiobase __attribute__((used)) = 0;
APTR KernelBase __attribute__((used)) = NULL;

#define MBoxBase (sd->mbox_base)

#define VCMB_BASE           (ARM_PERIIOBASE + 0xB880)
#define VCMB_PROPCHAN       8
#define VCTAG_REQ           0
#define VCTAG_GETCLKMAX     0x00030004
#define VCTAG_SETCLKSTATE   0x00038001
#define VCTAG_SETCLKRATE    0x00038002
#define BCM2711_PERIIOBASE  0xFE000000

#define PM_SPIN             1000000

static inline ULONG pm_rd(ULONG off)
{
    return *(volatile ULONG *)(ARM_PERIIOBASE + V3D_PM_OFFSET + off);
}

static inline void pm_wr(ULONG off, ULONG val)
{
    *(volatile ULONG *)(ARM_PERIIOBASE + V3D_PM_OFFSET + off)
        = V3D_PM_PASSWORD | val;
}

static inline ULONG asb_rd(ULONG off)
{
    return *(volatile ULONG *)(ARM_PERIIOBASE + V3D_ASB_OFFSET + off);
}

static inline void asb_wr(ULONG off, ULONG val)
{
    *(volatile ULONG *)(ARM_PERIIOBASE + V3D_ASB_OFFSET + off)
        = V3D_PM_PASSWORD | val;
}

/* Firmware clock id 5, state then rate: an enabled but unconfigured clock
 * has been seen parked at idle rates on the Pi 3, so ask for the maximum
 * and let the firmware's own thermal governor cap it. */
static void v3d_clock_on(struct V3DData *sd)
{
    volatile ULONG *msg = sd->mbox_msg;
    ULONG max = 0;

    ObtainSemaphore(&sd->mbox_lock);

    msg[0] = AROS_LE2LONG(8 * 4);
    msg[1] = AROS_LE2LONG(VCTAG_REQ);
    msg[2] = AROS_LE2LONG(VCTAG_SETCLKSTATE);
    msg[3] = AROS_LE2LONG(8);
    msg[4] = AROS_LE2LONG(8);
    msg[5] = AROS_LE2LONG(V3D_CLK_ID);
    msg[6] = AROS_LE2LONG(1);
    msg[7] = 0;
    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);
    bug("[V3D] clock state: 0x%08x\n", AROS_LE2LONG(msg[6]));

    msg[0] = AROS_LE2LONG(8 * 4);
    msg[1] = AROS_LE2LONG(VCTAG_REQ);
    msg[2] = AROS_LE2LONG(VCTAG_GETCLKMAX);
    msg[3] = AROS_LE2LONG(8);
    msg[4] = AROS_LE2LONG(4);
    msg[5] = AROS_LE2LONG(V3D_CLK_ID);
    msg[6] = 0;
    msg[7] = 0;
    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg)
        != (volatile unsigned int *)-1)
        max = AROS_LE2LONG(msg[6]);

    if (max)
    {
        msg[0] = AROS_LE2LONG(9 * 4);
        msg[1] = AROS_LE2LONG(VCTAG_REQ);
        msg[2] = AROS_LE2LONG(VCTAG_SETCLKRATE);
        msg[3] = AROS_LE2LONG(12);
        msg[4] = AROS_LE2LONG(12);
        msg[5] = AROS_LE2LONG(V3D_CLK_ID);
        msg[6] = AROS_LE2LONG(max);
        msg[7] = 0;                         /* skip turbo */
        msg[8] = 0;
        MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);
        bug("[V3D] clock rate: %u MHz\n",
            (unsigned)(AROS_LE2LONG(msg[6]) / 1000000));
    }

    ReleaseSemaphore(&sd->mbox_lock);
}

/*
 * V3D has no POWUP-style domain switch: writing POWUP into PM_GRAFX did
 * nothing on hardware and POWOK never came, because for this domain the
 * "power" is implicit in the clock, the reset and the ASB bridges. All
 * the PM register contributes is the reset bit.
 */
static BOOL v3d_reset_deassert(void)
{
    ULONG v = pm_rd(V3D_PM_GRAFX);

    bug("[V3D] PM_GRAFX at entry: 0x%08x\n", v);
    if (!(v & V3D_PM_V3DRSTN))
        pm_wr(V3D_PM_GRAFX, (v & 0xffffff) | V3D_PM_V3DRSTN);
    bug("[V3D] PM_GRAFX after reset deassert: 0x%08x\n",
        pm_rd(V3D_PM_GRAFX));
    return TRUE;
}

/* Unstall one ASB bridge: clear the stop request, wait for the ack to
 * follow it down. */
static BOOL v3d_asb_enable(ULONG reg, const char *name)
{
    ULONG v = asb_rd(reg), i;

    asb_wr(reg, (v & 0xffffff) & ~V3D_ASB_REQ_STOP);
    for (i = 0; i < PM_SPIN; i++)
        if (!(asb_rd(reg) & V3D_ASB_ACK))
            break;

    v = asb_rd(reg);
    bug("[V3D] ASB %s: 0x%08x\n", name, v);
    return !(v & V3D_ASB_ACK);
}

/*
 * Full block reset, for hang recovery: pull V3DRSTN, release it, and
 * unstall the ASB bridges again - the same authorities the power-up
 * sequence uses, so every step here is hardware-verified. The firmware
 * clock is left alone (a reset does not touch it), and BOs are firmware
 * allocations, so only the job in flight is lost.
 */
/* Stall one ASB bridge before a reset: request stop, wait for the ack.
 * Resetting with live bus traffic is what left the hub registers
 * reading garbage after repeated recoveries. */
static void v3d_asb_stall(ULONG reg)
{
    ULONG v = asb_rd(reg), i;

    asb_wr(reg, (v & 0xffffff) | V3D_ASB_REQ_STOP);
    for (i = 0; i < PM_SPIN; i++)
        if (asb_rd(reg) & V3D_ASB_ACK)
            break;
}

BOOL v3d_block_reset(void)
{
    ULONG i;

    v3d_asb_stall(V3D_ASB_V3D_S_CTRL);
    v3d_asb_stall(V3D_ASB_V3D_M_CTRL);

    pm_wr(V3D_PM_GRAFX, (pm_rd(V3D_PM_GRAFX) & 0xffffff) & ~V3D_PM_V3DRSTN);
    for (i = 0; i < PM_SPIN; i++)
        if (!(pm_rd(V3D_PM_GRAFX) & V3D_PM_V3DRSTN))
            break;

    pm_wr(V3D_PM_GRAFX, (pm_rd(V3D_PM_GRAFX) & 0xffffff) | V3D_PM_V3DRSTN);
    for (i = 0; i < PM_SPIN; i++)
        if (pm_rd(V3D_PM_GRAFX) & V3D_PM_V3DRSTN)
            break;

    bug("[V3D] block reset: PM_GRAFX=0x%08x\n", pm_rd(V3D_PM_GRAFX));

    return v3d_asb_enable(V3D_ASB_V3D_M_CTRL, "V3D_M")
        && v3d_asb_enable(V3D_ASB_V3D_S_CTRL, "V3D_S");
}

static int V3D_Init(LIBBASETYPEPTR LIBBASE)
{
    struct V3DData *sd = &LIBBASE->sd;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    /* V3D 4.2 exists on the BCM2711 alone. On anything older this offset
     * is the VideoCore IV V3D, which is vc4gallium's hardware. */
    if (__arm_periiobase != BCM2711_PERIIOBASE)
    {
        D(bug("[V3D] not a BCM2711 - not loading\n"));
        return FALSE;
    }

    if (!(sd->mbox_base = OpenResource("mbox.resource")))
        return FALSE;

    /* Own our cache lines; see <proto/mbox.h>. */
    sd->mbox_msg_raw = AllocMem(256 + (MBOX_MSG_ALIGN - 1),
                                MEMF_PUBLIC | MEMF_CLEAR);
    if (!sd->mbox_msg_raw)
        return FALSE;
    sd->mbox_msg = (volatile ULONG *)(((IPTR)sd->mbox_msg_raw
        + (MBOX_MSG_ALIGN - 1)) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    InitSemaphore(&sd->mbox_lock);
    InitSemaphore(&sd->bo_lock);
    InitSemaphore(&sd->job_lock);

    /* The CoreAPI table rides in on the attribute list at object
     * creation; without the base the tag id cannot even be computed. */
    sd->hiddGalliumAB = OOP_ObtainAttrBase(IID_Hidd_Gallium);
    if (!sd->hiddGalliumAB)
    {
        FreeMem(sd->mbox_msg_raw, 256 + (MBOX_MSG_ALIGN - 1));
        sd->mbox_msg_raw = NULL;
        return FALSE;
    }

    /* timer.device (UNIT_MICROHZ) for the GPU wait loop's microsleeps.
     * Non-fatal: without it the waits degrade to bounded spinning. */
    sd->gpu_timer_ok = FALSE;
    sd->gpu_timer_template.tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    sd->gpu_timer_template.tr_node.io_Message.mn_ReplyPort = NULL;
    sd->gpu_timer_template.tr_node.io_Message.mn_Length =
        sizeof(sd->gpu_timer_template);
    if (OpenDevice("timer.device", UNIT_MICROHZ,
                   (struct IORequest *)&sd->gpu_timer_template, 0) == 0)
        sd->gpu_timer_ok = TRUE;
    else
        bug("[V3D] timer.device unavailable - GPU waits will spin\n");

    /* Standard and vcgfx bitmap attributes, for the present fast paths
     * (DMA blit, flip, overlay). Non-fatal: without them presents fall
     * back to WritePixelArray. */
    sd->hiddBitMapAB = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    sd->hiddVCGfxBMAB = OOP_ObtainAttrBase(IID_Hidd_BitMap_VideoCore4);

    v3d_clock_on(sd);
    if (v3d_reset_deassert()
        && v3d_asb_enable(V3D_ASB_V3D_M_CTRL, "V3D_M")
        && v3d_asb_enable(V3D_ASB_V3D_S_CTRL, "V3D_S"))
    {
        if (!v3d_hw_init(sd))
            bug("[V3D] probe failed - GL falls back to softpipe\n");
    }
    else
        bug("[V3D] power-up failed - GL falls back to softpipe\n");

    return TRUE;
}

static int V3D_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct V3DData *sd = &LIBBASE->sd;

    if (sd->powered)
        v3d_hw_shutdown(sd);
    if (sd->hiddBitMapAB)
        OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    if (sd->hiddVCGfxBMAB)
        OOP_ReleaseAttrBase(IID_Hidd_BitMap_VideoCore4);
    if (sd->gpu_timer_ok)
    {
        CloseDevice((struct IORequest *)&sd->gpu_timer_template);
        sd->gpu_timer_ok = FALSE;
    }
    if (sd->hiddGalliumAB)
        OOP_ReleaseAttrBase(IID_Hidd_Gallium);
    if (sd->mbox_msg_raw)
        FreeMem(sd->mbox_msg_raw, 256 + (MBOX_MSG_ALIGN - 1));

    return TRUE;
}

ADD2INITLIB(V3D_Init, 0)
ADD2EXPUNGELIB(V3D_Expunge, 0)
