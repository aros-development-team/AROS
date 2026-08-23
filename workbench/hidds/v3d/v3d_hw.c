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
/*
 * Invalidate the GPU's read caches, in the order Mesa's own shim uses:
 * the L2, the texture L2 over its whole range, then the slice caches.
 * Without this a job runs against whatever those caches still hold, and
 * a frame comes out with only the parts that happened to miss.
 */
void v3d_flush_caches(struct V3DData *sd)
{
    if (!sd->powered)
        return;

    v3d_core_wr(sd, V3D_CTL_L2CACTL,
                V3D_L2CACTL_CLEAR | V3D_L2CACTL_ENABLE);

    v3d_core_wr(sd, V3D_CTL_L2TFLSTA, 0);
    v3d_core_wr(sd, V3D_CTL_L2TFLEND, ~0UL);
    v3d_core_wr(sd, V3D_CTL_L2TCACTL, V3D_L2TCACTL_FLUSH);

    v3d_core_wr(sd, V3D_CTL_SLCACTL, ~0UL);
}

BOOL v3d_submit_bin(struct V3DData *sd, ULONG start, ULONG end,
                    ULONG qma, ULONG qms, ULONG qts)
{
    static BOOL reported = FALSE;

    if (!sd->powered)
        return FALSE;

    if (qma)
    {
        v3d_core_wr(sd, V3D_CLE_CT0QMA, qma);
        v3d_core_wr(sd, V3D_CLE_CT0QMS, qms);
        v3d_core_wr(sd, V3D_CLE_CT0QTS, qts);
    }
    /* W1C the completion latch before starting, or the wait below sees
     * the previous frame's flush and hands the renderer an unflushed
     * binner. */
    v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_FLDONE);
    v3d_flush_caches(sd);
    sd->bin_end = end;
    v3d_core_wr(sd, V3D_CLE_CT0QBA, start);
    v3d_core_wr(sd, V3D_CLE_CT0QEA, end);

    /*
     * Read the queue back once. Whether the CLE ever saw a job is the
     * question under everything rendering black, and it splits three ways:
     * values that read back mean the registers are right and the executor
     * took the work; zeroes mean the writes are landing somewhere else -
     * these offsets came from the VideoCore IV and were never checked
     * against 4.x; and CT0CA moving means it ran.
     */
    if (!reported)
    {
        reported = TRUE;
        bug("[V3D] bin submit: wrote QBA=%08x QEA=%08x QMA=%08x QMS=%08x\n",
            start, end, qma, qms);
        bug("[V3D] bin readback: QBA=%08x QEA=%08x CT0CS=%08x CT0CA=%08x "
            "CT0EA=%08x\n",
            v3d_core_rd(sd, V3D_CLE_CT0QBA), v3d_core_rd(sd, V3D_CLE_CT0QEA),
            v3d_core_rd(sd, V3D_CLE_CT0CS), v3d_core_rd(sd, V3D_CLE_CT0CA),
            v3d_core_rd(sd, V3D_CLE_CT0EA));
        bug("[V3D] ident sanity: core IDENT0=%08x (V3D+4 = live core)\n",
            v3d_core_rd(sd, V3D_CTL_IDENT0));
    }
    return TRUE;
}

BOOL v3d_submit_render(struct V3DData *sd, ULONG start, ULONG end)
{
    if (!sd->powered)
        return FALSE;

    v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_FRDONE);
    v3d_flush_caches(sd);
    sd->render_end = end;
    v3d_core_wr(sd, V3D_CLE_CT1QBA, start);
    v3d_core_wr(sd, V3D_CLE_CT1QEA, end);
    return TRUE;
}

/*
 * Everything the GPU renders comes back as zeroes while CPU round-trips
 * through the same memory are perfect, which points at the GPU not
 * reaching that memory at all rather than at the drawing. On V3D 4.x
 * every access goes through the MMU - there is no bypass - so a disabled,
 * unconfigured one (MMU_CTL reading 0, as it does here) would produce
 * exactly this. Report the fault state once after the first job so the
 * question is settled by the hardware rather than by argument.
 */
static void v3d_report_once(struct V3DData *sd)
{
    static BOOL done = FALSE;

    if (done)
        return;
    done = TRUE;

    bug("[V3D] after first job: MMU_CTL=%08x ILLEGAL_ADDR=%08x "
        "hub INT_STS=%08x core INT_STS=%08x CT0CS=%08x CT1CS=%08x\n",
        v3d_hub_rd(sd, V3D_MMU_CTL), v3d_hub_rd(sd, V3D_MMU_ILLEGAL_ADDR),
        v3d_hub_rd(sd, V3D_HUB_INT_STS), v3d_core_rd(sd, V3D_CTL_INT_STS),
        v3d_core_rd(sd, V3D_CLE_CT0CS), v3d_core_rd(sd, V3D_CLE_CT1CS));
}

/*
 * Wait for the jobs that were actually submitted.
 *
 * Neither the RUN bit nor the active end address can carry this on its
 * own. A queue write starts the thread asynchronously, so RUN may not be
 * up at the first poll; and the active registers lag the queue, so
 * straight after a submit they still describe the PREVIOUS job - where
 * the current address has of course already reached the end. Either test
 * therefore reports "idle" for a job that has not begun, and the frame
 * gets presented out from under it. Only the first frame escapes, having
 * no predecessor, which is exactly how the flicker looked.
 *
 * Comparing against the end address this driver itself submitted has no
 * such ambiguity: it is reached when that specific job is done and at no
 * other time.
 *
 * Bounded: a wedged CLE degrades to a logged failure, not a hang.
 */
/*
 * The binner asks for more tile memory by raising OUTOMEM and stopping.
 * Nothing answers yet, so say so once rather than spinning silently to
 * the timeout - and say it from the polling path, because the interrupt
 * is not needed for this: vc4gallium services the same condition by
 * polling, its live IRQ path having made rendering worse rather than
 * better.
 */
void v3d_wait_idle(struct V3DData *sd)
{
    ULONG tries = 5000000;

    if (!sd->powered)
        return;

    while (--tries)
    {
        ULONG ints = v3d_core_rd(sd, V3D_CTL_INT_STS);
        BOOL busy = FALSE;


        /* The binner is done when it has flushed, which the completion
         * latch reports - reaching the end of the control list only means
         * the executor read it. */
        if (sd->bin_end && !(ints & V3D_INT_FLDONE))
            busy = TRUE;
        /* Same reasoning as the binner, and the reason its current
         * address is no guide: a render thread runs the per-tile lists,
         * so it spends the job inside the tile allocation and only the
         * completion latch says it is finished. */
        if (sd->render_end && !(ints & V3D_INT_FRDONE))
            busy = TRUE;

        if (!busy)
        {
            sd->bin_end = 0;
            sd->render_end = 0;
            v3d_report_once(sd);
            return;
        }
    }

    /* Say which thread is short of what: the end address each is waiting
     * for against where it actually is, and the raw latches. */
    {
        static ULONG n = 0;

        if (n++ < 4)
            bug("[V3D] stalled: bin CA=%08x want=%08x CS=%08x | "
                "render CA=%08x want=%08x CS=%08x | INT core=%08x hub=%08x\n",
                v3d_core_rd(sd, V3D_CLE_CT0CA), sd->bin_end,
                v3d_core_rd(sd, V3D_CLE_CT0CS),
                v3d_core_rd(sd, V3D_CLE_CT1CA), sd->render_end,
                v3d_core_rd(sd, V3D_CLE_CT1CS),
                v3d_core_rd(sd, V3D_CTL_INT_STS),
                v3d_hub_rd(sd, V3D_HUB_INT_STS));
    }
}
