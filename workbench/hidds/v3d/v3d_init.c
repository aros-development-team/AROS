/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    VideoCore VI (V3D) - driver initialisation.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/kernel.h>
#include <proto/mbox.h>

#include <hidd/gallium.h>

#include "v3d_intern.h"

#include LC_LIBDEFS_FILE

IPTR __arm_periiobase __attribute__((used)) = 0;
APTR KernelBase __attribute__((used)) = NULL;

#define MBoxBase (sd->mbox_base)

#define VCMB_BASE           (ARM_PERIIOBASE + 0xB880)
#define VCMB_PROPCHAN       8
#define VCTAG_REQ           0
#define VCTAG_SETPOWER      0x00028001
#define VCTAG_SETDOMAIN     0x00038030
#define VCPOWER_V3D         10
#define VCPOWER_STATE_ON    (1 << 0)
#define VCPOWER_STATE_WAIT  (1 << 1)
#define BCM2711_PERIIOBASE  0xFE000000

/*
 * Power the V3D domain up through the firmware. Two tags exist for this:
 * the classic device power tag vc4gallium uses on the Pi 3, and the
 * domain tag the Pi 4 firmware added. Ask with both - each is a no-op
 * where it is not understood - and let the register probe be the judge
 * of whether the block actually woke up.
 */
static void v3d_power_on(struct V3DData *sd)
{
    volatile ULONG *msg = sd->mbox_msg;

    ObtainSemaphore(&sd->mbox_lock);
    msg[0] = AROS_LE2LONG(8 * 4);
    msg[1] = AROS_LE2LONG(VCTAG_REQ);
    msg[2] = AROS_LE2LONG(VCTAG_SETPOWER);
    msg[3] = AROS_LE2LONG(8);
    msg[4] = AROS_LE2LONG(8);
    msg[5] = AROS_LE2LONG(VCPOWER_V3D);
    msg[6] = AROS_LE2LONG(VCPOWER_STATE_ON | VCPOWER_STATE_WAIT);
    msg[7] = 0;
    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);
    bug("[V3D] SETPOWER(V3D): state 0x%08x\n", AROS_LE2LONG(msg[6]));

    msg[0] = AROS_LE2LONG(8 * 4);
    msg[1] = AROS_LE2LONG(VCTAG_REQ);
    msg[2] = AROS_LE2LONG(VCTAG_SETDOMAIN);
    msg[3] = AROS_LE2LONG(8);
    msg[4] = AROS_LE2LONG(8);
    msg[5] = AROS_LE2LONG(VCPOWER_V3D);
    msg[6] = AROS_LE2LONG(1);
    msg[7] = 0;
    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);
    bug("[V3D] SET_DOMAIN(V3D): state 0x%08x\n", AROS_LE2LONG(msg[6]));
    ReleaseSemaphore(&sd->mbox_lock);
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

    v3d_power_on(sd);

    if (!v3d_hw_init(sd))
    {
        /* Not fatal: the hidd stays loadable and CreatePipeScreen keeps
         * answering NULL, which is the caller's softpipe fallback. */
        bug("[V3D] hardware probe failed - GL falls back to softpipe\n");
    }

    return TRUE;
}

static int V3D_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct V3DData *sd = &LIBBASE->sd;

    if (sd->powered)
        v3d_hw_shutdown(sd);
    if (sd->mbox_msg_raw)
        FreeMem(sd->mbox_msg_raw, 256 + (MBOX_MSG_ALIGN - 1));

    return TRUE;
}

ADD2INITLIB(V3D_Init, 0)
ADD2EXPUNGELIB(V3D_Expunge, 0)
