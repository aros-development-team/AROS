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
#include <exec/ports.h>
#include <proto/exec.h>

#include <string.h>

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

static inline ULONG v3d_now_us(void)
{
    return AROS_LE2LONG(*(volatile ULONG *)V3D_SYSTIMER_CLO);
}

/*
 * Scheduler-friendly microsleep for the wait loop: a timer.device
 * UNIT_MICROHZ request lets other tasks (input, mouse) run while the GPU
 * renders. Falls back to a wall-clock spin if the timer was unavailable
 * at init or the task has no free signal.
 */
static void v3d_gpu_nap(struct V3DData *sd, ULONG us)
{
    struct MsgPort port;
    struct timerequest tr;
    BYTE sig;

    if (!sd->gpu_timer_ok || (sig = AllocSignal(-1)) < 0)
    {
        ULONG start = v3d_now_us();

        while ((v3d_now_us() - start) < us)
            asm volatile("nop");
        return;
    }

    /* Zero the whole port: on SMP builds struct MsgPort carries a
     * spinlock, and stack garbage in it reads as "locked" - the timer
     * interrupt's ReplyMsg then spins forever in interrupt context. */
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

static void v3d_mmu_map_at(struct V3DData *sd, ULONG va, ULONG paddr,
                           ULONG size);

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

    /* What the firmware left in the overflow registers - the stale BPOA
     * is where an unsupplied overflow would spill. */
    bug("[V3D] PTB at entry: BPCA=%08x BPCS=%08x BPOA=%08x BPOS=%08x\n",
        v3d_core_rd(sd, V3D_PTB_BPCA), v3d_core_rd(sd, V3D_PTB_BPCS),
        v3d_core_rd(sd, V3D_PTB_BPOA), v3d_core_rd(sd, V3D_PTB_BPOS));

    v3d_core_wr(sd, V3D_PTB_BPOS, 0);

    /* Two overflow allocations for the driver's life (armed alternately
     * per bin job); hw_init also runs on hang recovery, so don't
     * allocate twice. Mapped for the GPU below, once the page table
     * exists - BPOA takes the VA. */
    for (i = 0; i < 2; i++)
    {
        if (!sd->overflow_handle[i])
            sd->overflow_handle[i] = v3d_gpu_mem_alloc(sd, V3D_OVERFLOW_SIZE,
                                                       4096,
                                                       &sd->overflow_paddr[i]);
        if (!sd->overflow_handle[i])
            bug("[V3D] no overflow BO %u - binner overflow will stall\n",
                (unsigned)i);
    }

    /*
     * Enable the MMU and run every BO at a LOW GPU virtual address:
     * feeding the hardware identity-mapped ~1GB physicals made the PTB
     * emit a load-bearing stream at a low stray address (an internal
     * truncation/banking path nothing else exercises), which ate exec's
     * LVO tables, the boot page tables and the IRQ lists at fullscreen
     * tile counts. Everything unmapped faults into the scratch page and
     * latches VIO_ADDR. PT entries: 32-bit, pfn low, VALID|WRITEABLE;
     * zeroed = invalid, which VCMEM_ZERO gives for free. Mappings are
     * made per-BO at CREATE and survive a recovery reset (the PT lives
     * in firmware memory).
     */
    if (!sd->mmu_pt_handle)
    {
        sd->mmu_pt_handle = v3d_gpu_mem_alloc(sd, V3D_MMU_PT_SIZE, 4096,
                                              &sd->mmu_pt_paddr);
        sd->mmu_scratch_handle = v3d_gpu_mem_alloc(sd, 4096, 4096,
                                                   &sd->mmu_scratch_paddr);
    }

    if (sd->mmu_pt_handle && sd->mmu_scratch_handle)
    {
        v3d_hub_wr(sd, V3D_MMU_PT_PA_BASE, sd->mmu_pt_paddr >> 12);
        v3d_hub_wr(sd, V3D_MMU_ILLEGAL_ADDR,
                   V3D_MMU_ILLEGAL_ADDR_ENABLE
                   | (sd->mmu_scratch_paddr >> 12));
        v3d_hub_wr(sd, V3D_MMUC_CONTROL,
                   V3D_MMUC_CONTROL_ENABLE | V3D_MMUC_CONTROL_FLUSH);
        /* No ABORT bits: the PTB's stray stream (VIO_ID=0x20 at low-RAM
         * addresses, source still unidentified) fires on every big bin
         * job, and aborting killed each frame - black screen. Redirect-
         * to-scratch absorbs the strays harmlessly; INT latches keep
         * them visible in the logs. */
        v3d_hub_wr(sd, V3D_MMU_CTL,
                   V3D_MMU_CTL_ENABLE | V3D_MMU_CTL_TLB_CLEAR
                   | V3D_MMU_CTL_PT_INVALID_ENABLE
                   | V3D_MMU_CTL_PT_INVALID_INT
                   | V3D_MMU_CTL_WRITE_VIOLATION_INT
                   | V3D_MMU_CTL_CAP_EXCEEDED_INT);
        bug("[V3D] MMU on: PT@%08x scratch@%08x CTL=%08x DEBUG_INFO=%08x\n",
            sd->mmu_pt_paddr, sd->mmu_scratch_paddr,
            v3d_hub_rd(sd, V3D_MMU_CTL),
            v3d_hub_rd(sd, V3D_MMU_DEBUG_INFO));

        for (i = 0; i < 2; i++)
            if (sd->overflow_handle[i] && !sd->overflow_va[i])
                sd->overflow_va[i] = v3d_mmu_map(sd, sd->overflow_paddr[i],
                                                 V3D_OVERFLOW_SIZE);

        /*
         * PTB quirk landing zone. The binner emits a stream at a FIXED
         * low VA (~0x11000-0x12000 scaled, MMU-measured, independent of
         * every base register - all five verified by readback) and the
         * renderer starves when those writes are discarded. A VA
         * allocator that hands out the bottom of the address space first
         * would have somebody's buffer sitting there by accident and
         * never notice; ours folds addresses from the physical heap
         * instead, so the band has to be given real memory explicitly.
         */
        if (!sd->ptb_quirk_handle)
            sd->ptb_quirk_handle = v3d_gpu_mem_alloc(sd, V3D_PTB_QUIRK_SIZE,
                                                     4096,
                                                     &sd->ptb_quirk_paddr);
        if (sd->ptb_quirk_handle)
            v3d_mmu_map_at(sd, 0, sd->ptb_quirk_paddr, V3D_PTB_QUIRK_SIZE);
    }
    else
        bug("[V3D] MMU page table alloc failed - running unprotected\n");

    sd->powered = TRUE;
    return TRUE;
}

/*
 * Map a BO for the GPU at its low virtual address (V3D_GPU_VA: a
 * stateless, collision-free fold of the physical address into the
 * bottom 512MB) and return that VA. The PT lives in uncached firmware
 * memory, so the walker sees the entries as soon as the TLB is cleared;
 * creates and frees only happen with the GPU idle (synchronous submit),
 * so a full clear is safe and simple.
 */
static void v3d_mmu_flush(struct V3DData *sd)
{
    v3d_hub_wr(sd, V3D_MMUC_CONTROL,
               V3D_MMUC_CONTROL_ENABLE | V3D_MMUC_CONTROL_FLUSH);
    v3d_hub_wr(sd, V3D_MMU_CTL,
               v3d_hub_rd(sd, V3D_MMU_CTL) | V3D_MMU_CTL_TLB_CLEAR);
}

static void v3d_mmu_map_at(struct V3DData *sd, ULONG va, ULONG paddr,
                           ULONG size)
{
    volatile ULONG *pt = (volatile ULONG *)(IPTR)sd->mmu_pt_paddr;
    ULONG pages = (size + 4095) >> 12;
    ULONG i;

    for (i = 0; i < pages; i++)
        pt[(va >> 12) + i] = V3D_PTE_VALID | V3D_PTE_WRITEABLE
                           | ((paddr >> 12) + i);
    v3d_mmu_flush(sd);
}

ULONG v3d_mmu_map(struct V3DData *sd, ULONG paddr, ULONG size)
{
    if (!sd->mmu_pt_handle)
        return V3D_GPU_ADDR(paddr);    /* unprotected fallback */

    v3d_mmu_map_at(sd, V3D_GPU_VA(paddr), paddr, size);
    return V3D_GPU_VA(paddr);
}

void v3d_mmu_unmap(struct V3DData *sd, ULONG gpu_va, ULONG size)
{
    volatile ULONG *pt = (volatile ULONG *)(IPTR)sd->mmu_pt_paddr;
    ULONG pages = (size + 4095) >> 12;
    ULONG i;

    if (!sd->mmu_pt_handle)
        return;

    for (i = 0; i < pages; i++)
        pt[(gpu_va >> 12) + i] = 0;
    v3d_mmu_flush(sd);
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

/*
 * Advance the pipeline from the completion latches. Caller holds
 * job_lock. The binner is done when it has FLUSHED (FLDONE) - reaching
 * the end of its control list only means the executor read it - and the
 * render retires on FRDONE. The stashed render is kicked only when its
 * own bin has flushed AND the previous render has retired: CT1 ignores
 * queue writes while running, and a premature kick silently drops the
 * frame.
 */
static void v3d_service(struct V3DData *sd)
{
    ULONG ints = v3d_core_rd(sd, V3D_CTL_INT_STS);

    if (sd->bin_running && (ints & V3D_INT_FLDONE))
    {
        v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_FLDONE);
        sd->bin_running = FALSE;
        /* A bin-only submission is finished at the flush */
        if (!sd->pending_rcl_end && sd->finished_seqno < sd->bin_seqno)
            sd->finished_seqno = sd->bin_seqno;
    }

    if (sd->render_running && (ints & V3D_INT_FRDONE))
    {
        v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_FRDONE);
        sd->render_running = FALSE;
        sd->finished_seqno = sd->render_seqno;
    }

    if (sd->pending_rcl_end && !sd->bin_running && !sd->render_running)
    {
        v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_FRDONE);
        v3d_flush_caches(sd);
        sd->render_end = sd->pending_rcl_end;
        sd->render_seqno = sd->pending_seqno;
        sd->render_running = TRUE;
        v3d_core_wr(sd, V3D_CLE_CT1QBA, sd->pending_rcl_start);
        v3d_core_wr(sd, V3D_CLE_CT1QEA, sd->pending_rcl_end);
        sd->pending_rcl_start = 0;
        sd->pending_rcl_end = 0;
    }
}

static BOOL v3d_pipeline_idle(struct V3DData *sd)
{
    return !sd->bin_running && !sd->render_running
        && !sd->pending_rcl_end && sd->finished_seqno == sd->seqno;
}

/*
 * One line after the first completed job: the MMU violation latches and
 * VIO_ADDR. Nonzero status bits here mean the PTB's fixed-low-VA quirk
 * stream has grown past the landing zone (see hw_init) - enlarge it.
 */
static void v3d_report_once(struct V3DData *sd)
{
    static BOOL done = FALSE;

    if (done)
        return;
    done = TRUE;

    bug("[V3D] after first job: MMU_CTL=%08x VIO_ADDR=%08x VIO_ID=%08x "
        "hub INT_STS=%08x core INT_STS=%08x CT0CS=%08x CT1CS=%08x\n",
        v3d_hub_rd(sd, V3D_MMU_CTL), v3d_hub_rd(sd, V3D_MMU_VIO_ADDR),
        v3d_hub_rd(sd, V3D_MMU_VIO_ID),
        v3d_hub_rd(sd, V3D_HUB_INT_STS), v3d_core_rd(sd, V3D_CTL_INT_STS),
        v3d_core_rd(sd, V3D_CLE_CT0CS), v3d_core_rd(sd, V3D_CLE_CT1CS));
    /* BPCA moving away from BPOA means the overflow was consumed. */
    bug("[V3D] after first job: BPCA=%08x BPCS=%08x BPOA=%08x BPOS=%08x\n",
        v3d_core_rd(sd, V3D_PTB_BPCA), v3d_core_rd(sd, V3D_PTB_BPCS),
        v3d_core_rd(sd, V3D_PTB_BPOA), v3d_core_rd(sd, V3D_PTB_BPOS));
}

/*
 * Timeout path: dump what each thread was short of, then recover - or
 * every later frame walks into the same timeout. The V3D 4.x CLE has no
 * known per-thread reset bit, so reset the whole block through the
 * measured power-up path and reprobe; the pipeline state is dropped
 * with the dead jobs. Caller holds job_lock.
 */
static void v3d_recover(struct V3DData *sd)
{
    static ULONG n = 0;

    if (n++ < 4)
        bug("[V3D] stalled: bin CA=%08x want=%08x CS=%08x run=%d | "
            "render CA=%08x want=%08x CS=%08x run=%d pend=%d | "
            "INT core=%08x hub=%08x MMU_CTL=%08x VIO_ADDR=%08x "
            "seq=%u fin=%u\n",
            v3d_core_rd(sd, V3D_CLE_CT0CA), sd->bin_end,
            v3d_core_rd(sd, V3D_CLE_CT0CS), (int)sd->bin_running,
            v3d_core_rd(sd, V3D_CLE_CT1CA), sd->render_end,
            v3d_core_rd(sd, V3D_CLE_CT1CS), (int)sd->render_running,
            (int)(sd->pending_rcl_end != 0),
            v3d_core_rd(sd, V3D_CTL_INT_STS),
            v3d_hub_rd(sd, V3D_HUB_INT_STS),
            v3d_hub_rd(sd, V3D_MMU_CTL),
            v3d_hub_rd(sd, V3D_MMU_VIO_ADDR),
            (unsigned)sd->seqno, (unsigned)sd->finished_seqno);

    sd->bin_running = FALSE;
    sd->render_running = FALSE;
    sd->pending_rcl_start = 0;
    sd->pending_rcl_end = 0;
    sd->finished_seqno = sd->seqno;
    sd->bin_end = 0;
    sd->render_end = 0;
    sd->powered = FALSE;

    /* A recovery fuse: when the workload rewedges the GPU immediately,
     * endless reset cycles just churn a half-broken hub. Give up after
     * a few and let GL degrade; session teardown resets the fuse. */
    if (++sd->recoveries > 4)
    {
        if (sd->recoveries == 5)
            bug("[V3D] too many recoveries - GPU disabled for this "
                "session\n");
        return;
    }

    bug("[V3D] recovering: resetting the V3D block\n");
    if (!v3d_block_reset() || !v3d_hw_init(sd))
        bug("[V3D] recovery failed - GPU stays down\n");
}

/*
 * Run the service machine until `until` is satisfied, with the tiered
 * wait (spin for µs-precise completion of short jobs, then timer naps
 * so the rest of the system runs while the GPU renders), bounded by the
 * hang timeout. Caller holds job_lock.
 */
static BOOL v3d_wait_for(struct V3DData *sd, BOOL (*until)(struct V3DData *))
{
    ULONG start = v3d_now_us();

    for (;;)
    {
        ULONG waited;

        v3d_service(sd);
        if (until(sd))
            return TRUE;

        waited = v3d_now_us() - start;
        if (waited >= V3D_GPUWAIT_TIMEOUT_US)
        {
            v3d_recover(sd);
            return FALSE;
        }
        if (waited >= V3D_GPUWAIT_SPIN_US)
            v3d_gpu_nap(sd, V3D_GPUWAIT_NAP_US);
    }
}

static BOOL v3d_slot_free(struct V3DData *sd)
{
    return !sd->bin_running && !sd->pending_rcl_end;
}

/*
 * Submit one frame: kick the bin job and stash the render job; the
 * service machine hands the renderer over on FLDONE. Returns as soon as
 * the bin is kicked, so the caller builds the next frame while this one
 * renders - waiting only when the previous frame's render has not yet
 * been handed its control list.
 */
BOOL v3d_submit_cl(struct V3DData *sd, ULONG bcl_start, ULONG bcl_end,
                   ULONG qma, ULONG qms, ULONG qts,
                   ULONG rcl_start, ULONG rcl_end)
{
    ULONG seqno;

    ObtainSemaphore(&sd->job_lock);

    if (!sd->powered || !v3d_wait_for(sd, v3d_slot_free) || !sd->powered)
    {
        ReleaseSemaphore(&sd->job_lock);
        return FALSE;
    }

    seqno = ++sd->seqno;

    if (bcl_start != bcl_end)
    {
        if (qma)
        {
            v3d_core_wr(sd, V3D_CLE_CT0QMA, qma);
            v3d_core_wr(sd, V3D_CLE_CT0QMS, qms);
        }
        if (qts)
            v3d_core_wr(sd, V3D_CLE_CT0QTS, V3D_CLE_CT0QTS_ENABLE | qts);
        /* Alternate overflow supplies: the previous frame's render may
         * still read tile lists in the other one. */
        if (sd->overflow_va[seqno & 1])
        {
            v3d_core_wr(sd, V3D_PTB_BPOA, sd->overflow_va[seqno & 1]);
            v3d_core_wr(sd, V3D_PTB_BPOS, V3D_OVERFLOW_SIZE);
        }
        /* The slot wait consumed the previous FLDONE; W1C defensively so
         * the flush of THIS bin is what the service machine sees. */
        v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_FLDONE);
        v3d_flush_caches(sd);
        sd->bin_end = bcl_end;
        sd->bin_seqno = seqno;
        sd->bin_running = TRUE;
        v3d_core_wr(sd, V3D_CLE_CT0QBA, bcl_start);
        v3d_core_wr(sd, V3D_CLE_CT0QEA, bcl_end);
        D(bug("[V3D] bin submit %u: QBA=%08x QEA=%08x\n", (unsigned)seqno,
              bcl_start, bcl_end));
    }

    if (rcl_start != rcl_end)
    {
        sd->pending_rcl_start = rcl_start;
        sd->pending_rcl_end = rcl_end;
        sd->pending_seqno = seqno;
    }
    else if (bcl_start == bcl_end)
        sd->finished_seqno = seqno;     /* empty submission */

    /* An RCL with no BCL (or a bin that flushed instantly) can go now */
    v3d_service(sd);

    ReleaseSemaphore(&sd->job_lock);
    return TRUE;
}

void v3d_wait_idle(struct V3DData *sd)
{
    ObtainSemaphore(&sd->job_lock);
    if (sd->powered)
    {
        if (v3d_wait_for(sd, v3d_pipeline_idle))
            v3d_report_once(sd);
    }
    ReleaseSemaphore(&sd->job_lock);
}
