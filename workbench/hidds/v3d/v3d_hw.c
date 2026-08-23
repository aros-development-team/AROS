/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    VideoCore VI (V3D 4.2) - hardware probe and job submission.

    Phase A is the probe: identify the block and record what state the
    firmware left it in - above all whether the MMU is live, which decides
    how buffer addresses reach the GPU. Submission stays a synchronous
    CLE kick with a bounded poll, the same baseline vc4gallium started
    from, until the probe output from real hardware settles the open
    questions.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <proto/exec.h>

#include "v3d_intern.h"

static inline ULONG v3d_hub_rd(struct V3DData *sd, ULONG off)
{
    return *(volatile ULONG *)(sd->hub_base + off);
}

static inline void v3d_hub_wr(struct V3DData *sd, ULONG off, ULONG val)
{
    *(volatile ULONG *)(sd->hub_base + off) = val;
}

static inline ULONG v3d_core_rd(struct V3DData *sd, ULONG off)
{
    return *(volatile ULONG *)(sd->core0_base + off);
}

static inline void v3d_core_wr(struct V3DData *sd, ULONG off, ULONG val)
{
    *(volatile ULONG *)(sd->core0_base + off) = val;
}

BOOL v3d_hw_init(struct V3DData *sd)
{
    ULONG i;

    sd->hub_base   = ARM_PERIIOBASE + V3D_HUB_OFFSET;
    sd->core0_base = ARM_PERIIOBASE + V3D_CORE0_OFFSET;

    for (i = 0; i < 4; i++)
        sd->hub_ident[i] = v3d_hub_rd(sd, V3D_HUB_IDENT0 + 4 * i);
    for (i = 0; i < 3; i++)
        sd->core_ident[i] = v3d_core_rd(sd, V3D_CTL_IDENT0 + 4 * i);

    /* For the log only - Mesa parses the raw idents itself through
     * GET_PARAM. Low nibble is the major: 0x24 on hardware = V3D 4.2. */
    sd->ver = (sd->hub_ident[1] & 0xf) * 10 + ((sd->hub_ident[1] >> 4) & 0xf);

    bug("[V3D] hub ident %08x %08x %08x %08x core %08x %08x %08x -> V3D %u.%u\n",
        sd->hub_ident[0], sd->hub_ident[1], sd->hub_ident[2],
        sd->hub_ident[3], sd->core_ident[0], sd->core_ident[1],
        sd->core_ident[2], (unsigned)(sd->ver / 10), (unsigned)(sd->ver % 10));

    /* Measured on hardware: the hub signs itself "VHUB" and the core
     * "V3D"+4. 0xdeadbeef everywhere means the wake-up sequence missed a
     * step; all-zeroes/all-ones is a dead bus. */
    if (sd->hub_ident[0] == 0 || sd->hub_ident[0] == 0xffffffff
        || sd->hub_ident[0] == 0xdeadbeef)
    {
        bug("[V3D] hub not responding - unpowered?\n");
        return FALSE;
    }
    if (sd->hub_ident[0] != 0x42554856)
        bug("[V3D] no VHUB signature in IDENT0 - read the dump carefully\n");

    /* The address-path question phase B hangs on: an enabled MMU means
     * BO addresses are GPU-virtual through a firmware page table, a
     * disabled one means the masked bus address works as on VideoCore IV. */
    bug("[V3D] MMU_CTL=%08x PT_PA_BASE=%08x AXICFG=%08x UIFCFG=%08x\n",
        v3d_hub_rd(sd, V3D_MMU_CTL), v3d_hub_rd(sd, V3D_MMU_PT_PA_BASE),
        v3d_hub_rd(sd, V3D_HUB_AXICFG), v3d_hub_rd(sd, V3D_HUB_UIFCFG));

    /* Everything masked until an interrupt path exists. */
    v3d_hub_wr(sd, V3D_HUB_INT_MSK_SET, 0xffffffff);
    v3d_hub_wr(sd, V3D_HUB_INT_CLR, 0xffffffff);
    v3d_core_wr(sd, V3D_CTL_INT_MSK_SET, 0xffffffff);
    v3d_core_wr(sd, V3D_CTL_INT_CLR, 0xffffffff);

    sd->powered = TRUE;
    return TRUE;
}

void v3d_hw_shutdown(struct V3DData *sd)
{
    v3d_hub_wr(sd, V3D_HUB_INT_MSK_SET, 0xffffffff);
    sd->powered = FALSE;
}

/*
 * Kick the control lists and let the CLE run: thread 0 bins, thread 1
 * renders, and each starts when its end address lands. The binner's tile
 * alloc memory is not optional on 4.1+ - Mesa allocates it and passes the
 * three values with the submit. No cache work anywhere: every BO lives in
 * the uncached GPU region.
 */
BOOL v3d_submit_bin(struct V3DData *sd, ULONG start, ULONG end,
                    ULONG qma, ULONG qms, ULONG qts)
{
    if (!sd->powered)
        return FALSE;

    if (qma)
    {
        v3d_core_wr(sd, V3D_CLE_CT0QMA, qma);
        v3d_core_wr(sd, V3D_CLE_CT0QMS, qms);
        v3d_core_wr(sd, V3D_CLE_CT0QTS, qts);
    }
    v3d_core_wr(sd, V3D_CLE_CT0QBA, start);
    v3d_core_wr(sd, V3D_CLE_CT0QEA, end);
    return TRUE;
}

BOOL v3d_submit_render(struct V3DData *sd, ULONG start, ULONG end)
{
    if (!sd->powered)
        return FALSE;

    v3d_core_wr(sd, V3D_CLE_CT1QBA, start);
    v3d_core_wr(sd, V3D_CLE_CT1QEA, end);
    return TRUE;
}

/* Bounded: a wedged CLE degrades to a logged failure, not a hang. */
void v3d_wait_idle(struct V3DData *sd)
{
    ULONG tries = 5000000;

    if (!sd->powered)
        return;

    while (--tries)
    {
        if (!(v3d_core_rd(sd, V3D_CLE_CT0CS) & V3D_CLE_CTCS_RUN)
            && !(v3d_core_rd(sd, V3D_CLE_CT1CS) & V3D_CLE_CTCS_RUN))
            return;
    }

    bug("[V3D] CLE stuck: CT0CS=%08x CT0CA=%08x CT1CS=%08x CT1CA=%08x\n",
        v3d_core_rd(sd, V3D_CLE_CT0CS), v3d_core_rd(sd, V3D_CLE_CT0CA),
        v3d_core_rd(sd, V3D_CLE_CT1CS), v3d_core_rd(sd, V3D_CLE_CT1CA));
}
