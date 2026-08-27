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
#include <proto/kernel.h>

#include <string.h>

#include "v3d_intern.h"

extern APTR KernelBase;

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
static void v3d_gpu_nap(struct V3DData *sd, ULONG us, ULONG wakesigs)
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

    /* The completion interrupt gets to cut the nap short; the timer is
     * then only the safety net for an interrupt that never arrives. */
    SendIO((struct IORequest *)&tr);
    Wait((1UL << sig) | wakesigs);
    AbortIO((struct IORequest *)&tr);
    WaitIO((struct IORequest *)&tr);

    FreeSignal(sig);
}

static void v3d_mmu_map_at(struct V3DData *sd, ULONG va, ULONG paddr,
                           ULONG size);
static void v3d_irq_handler(struct V3DData *sd, struct ExecBase *sysBase);

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

    
    /* Mask and clear everything, then arm just the sources the service
     * machine acts on. The hub's stay masked - nothing there is wanted,
     * and its MMU violation latches are read from the logs instead. */
    v3d_hub_wr(sd, V3D_HUB_INT_MSK_SET, 0xffffffff);
    v3d_hub_wr(sd, V3D_HUB_INT_CLR, 0xffffffff);
    v3d_core_wr(sd, V3D_CTL_INT_MSK_SET, 0xffffffff);
    v3d_core_wr(sd, V3D_CTL_INT_CLR, 0xffffffff);

#if V3D_IRQ_ENABLE
    if (!sd->irq_handle)
    {
        sd->waiter = NULL;
        sd->waiter_sig = -1;
        sd->irq_handle = KrnAddIRQHandler(V3D_IRQ, v3d_irq_handler,
                                          sd, SysBase);
    }
    if (sd->irq_handle)
    {
        /* Clearing a mask bit enables that source. */
        v3d_core_wr(sd, V3D_CTL_INT_MSK_CLR,
                    V3D_INT_FLDONE | V3D_INT_FRDONE | V3D_INT_OUTOMEM);
        bug("[V3D] completion IRQ armed on INTID %u (MSK_STS=%08x)\n",
            (unsigned)V3D_IRQ, v3d_core_rd(sd, V3D_CTL_INT_MSK_STS));
    }
    else
        bug("[V3D] no IRQ handler - polling only\n");
#endif

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
     * Enable the MMU and map every BO at its physical address (see
     * V3D_GPU_VA). Everything unmapped faults into the scratch page and
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
 * Map a BO for the GPU at its physical address (V3D_GPU_VA: a stateless
 * identity map) and return that VA. The PT lives in uncached firmware
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
    /* Mask before the handler goes: an interrupt arriving after it has
     * been removed would be nobody's. */
    v3d_core_wr(sd, V3D_CTL_INT_MSK_SET, 0xffffffff);
    v3d_hub_wr(sd, V3D_HUB_INT_MSK_SET, 0xffffffff);
    if (sd->irq_handle)
    {
        KrnRemIRQHandler(sd->irq_handle);
        sd->irq_handle = NULL;
    }
    sd->waiter = NULL;
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
    ULONG ints;

    /* Runs from the interrupt as well as the wait paths, so the state
     * machine below is a critical section. Disable() suffices on UP; SMP
     * would need job_lock to become a spinlock the handler can take. */
    Disable();
    ints = v3d_core_rd(sd, V3D_CTL_INT_STS);

    /*
     * OUTOMEM: the binner outran the supply armed at submit and has
     * stopped. Only record the latch here - the one buffer left to hand
     * over may still be feeding the previous render, so the refill waits
     * for that render to retire.
     */
    if (ints & V3D_INT_OUTOMEM)
    {
        v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_OUTOMEM);
        sd->oom_pending = TRUE;
        sd->oom_events++;
    }
    if (ints & V3D_INT_SPILLUSE)
    {
        v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_SPILLUSE);
        sd->spill_events++;
    }

    if (sd->oom_pending && !sd->render_running)
    {
        ULONG idx = (sd->bin_seqno + 1) & 1;

        if (sd->oom_handed || !sd->overflow_va[idx])
        {
            /* Second exhaustion in one job (or no supply at all): let it
             * stall. The wait times out, dumps the state and recovers -
             * better than feeding memory a live render still reads. */
            if (sd->oom_events <= 4)
                bug("[V3D] binner out of tile memory twice in one job - "
                    "stalling (events=%u)\n", (unsigned)sd->oom_events);
            sd->oom_pending = FALSE;
        }
        else
        {
            v3d_core_wr(sd, V3D_PTB_BPOA, sd->overflow_va[idx]);
            v3d_core_wr(sd, V3D_PTB_BPOS, V3D_OVERFLOW_SIZE);
            sd->oom_handed = TRUE;
            sd->oom_pending = FALSE;
            if (sd->oom_events <= 8)
                bug("[V3D] binner out of tile memory: fed supply %u "
                    "(event %u)\n", (unsigned)idx, (unsigned)sd->oom_events);
        }
    }

    if (sd->bin_running && (ints & V3D_INT_FLDONE))
    {
        v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_FLDONE);
        sd->bin_running = FALSE;
        sd->bin_flushed_seqno = sd->bin_seqno;
#if V3D_PROFILE
        sd->prof_bin_acc += v3d_now_us() - sd->prof_bin_kick;
#endif
        /* A bin-only submission is finished at the flush */
        if (sd->bin_only && sd->finished_seqno < sd->bin_seqno)
            sd->finished_seqno = sd->bin_seqno;
    }

    if (sd->render_running && (ints & V3D_INT_FRDONE))
    {
        v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_FRDONE);
        sd->render_running = FALSE;
        sd->finished_seqno = sd->render_seqno;
#if V3D_PROFILE
        sd->prof_render_acc += v3d_now_us() - sd->prof_render_kick;
        /* Only snapshot here: this runs in interrupt context, and a
         * serial line would hold interrupts off for milliseconds. The
         * next submit prints it. */
        if (++sd->prof_jobs >= V3D_PROF_PERIOD)
        {
            sd->prof_rep_bin = sd->prof_bin_acc / sd->prof_jobs;
            sd->prof_rep_render = sd->prof_render_acc / sd->prof_jobs;
            sd->prof_rep_jobs = sd->prof_jobs;
            sd->prof_bin_acc = sd->prof_render_acc = sd->prof_jobs = 0;
        }
#endif
    }

    /* The oldest stashed job whose own bin has flushed. Deliberately not
     * waiting for the binner to be idle: bin N+1 running while render N
     * starts is the point of the queue. */
    if (sd->rcl_count && !sd->render_running
        && sd->rcl_q[sd->rcl_head].seqno <= sd->bin_flushed_seqno)
    {
        ULONG start = sd->rcl_q[sd->rcl_head].start;
        ULONG end = sd->rcl_q[sd->rcl_head].end;

        sd->render_seqno = sd->rcl_q[sd->rcl_head].seqno;
        sd->rcl_head = (sd->rcl_head + 1) % V3D_RCL_QUEUE;
        sd->rcl_count--;

        v3d_core_wr(sd, V3D_CTL_INT_CLR, V3D_INT_FRDONE);
        v3d_flush_caches(sd);
        sd->render_end = end;
        sd->render_running = TRUE;
#if V3D_PROFILE
        sd->prof_render_kick = v3d_now_us();
#endif
        v3d_core_wr(sd, V3D_CLE_CT1QBA, start);
        v3d_core_wr(sd, V3D_CLE_CT1QEA, end);
    }

    Enable();
}

/* The completion interrupt. Servicing from here is what keeps the
 * renderer fed - with the present ring no longer waiting, nothing else
 * would look until the next submit. */
static void v3d_irq_handler(struct V3DData *sd, struct ExecBase *sysBase)
{
    struct Task *t;

    v3d_service(sd);

    t = sd->waiter;
    if (t && sd->waiter_sig >= 0)
        Signal(t, 1UL << sd->waiter_sig);
}

static BOOL v3d_pipeline_idle(struct V3DData *sd)
{
    return !sd->bin_running && !sd->render_running
        && !sd->rcl_count && sd->finished_seqno == sd->seqno;
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
    bug("[V3D] after first job: BPCA=%08x BPCS=%08x BPOA=%08x BPOS=%08x "
        "oom=%u spill=%u\n",
        v3d_core_rd(sd, V3D_PTB_BPCA), v3d_core_rd(sd, V3D_PTB_BPCS),
        v3d_core_rd(sd, V3D_PTB_BPOA), v3d_core_rd(sd, V3D_PTB_BPOS),
        (unsigned)sd->oom_events, (unsigned)sd->spill_events);
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
            "render CA=%08x want=%08x CS=%08x run=%d rclq=%d | "
            "INT core=%08x hub=%08x MMU_CTL=%08x VIO_ADDR=%08x "
            "seq=%u fin=%u oom=%u/%u spill=%u BPCS=%08x\n",
            v3d_core_rd(sd, V3D_CLE_CT0CA), sd->bin_end,
            v3d_core_rd(sd, V3D_CLE_CT0CS), (int)sd->bin_running,
            v3d_core_rd(sd, V3D_CLE_CT1CA), sd->render_end,
            v3d_core_rd(sd, V3D_CLE_CT1CS), (int)sd->render_running,
            (int)sd->rcl_count,
            v3d_core_rd(sd, V3D_CTL_INT_STS),
            v3d_hub_rd(sd, V3D_HUB_INT_STS),
            v3d_hub_rd(sd, V3D_MMU_CTL),
            v3d_hub_rd(sd, V3D_MMU_VIO_ADDR),
            (unsigned)sd->seqno, (unsigned)sd->finished_seqno,
            (unsigned)sd->oom_events, (unsigned)sd->oom_handed,
            (unsigned)sd->spill_events,
            v3d_core_rd(sd, V3D_PTB_BPCS));

    sd->bin_running = FALSE;
    sd->render_running = FALSE;
    sd->rcl_head = 0;
    sd->rcl_count = 0;
    sd->bin_flushed_seqno = sd->seqno;
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
    BOOL ret = TRUE;
    BYTE sig = -1;

    for (;;)
    {
        ULONG waited;

        /* Still serviced here as well as from the handler: harmless
         * (one register read), and it is the whole mechanism when the
         * interrupt is disabled or never arrives. */
        v3d_service(sd);
        if (until(sd))
            break;

        waited = v3d_now_us() - start;
        if (waited >= V3D_GPUWAIT_TIMEOUT_US)
        {
            v3d_recover(sd);
            ret = FALSE;
            break;
        }
        if (waited < V3D_GPUWAIT_SPIN_US)
            continue;       /* short jobs never leave the spin window */

        /* Past the spin window: register for the completion interrupt
         * and sleep on it, with the timer nap as the safety net. Only
         * one waiter exists at a time - every caller holds job_lock. */
        if (sig < 0 && sd->irq_handle && (sig = AllocSignal(-1)) >= 0)
        {
            Disable();
            sd->waiter_sig = sig;
            sd->waiter = FindTask(NULL);
            Enable();
            continue;       /* recheck: it may have finished meanwhile */
        }

        v3d_gpu_nap(sd, V3D_GPUWAIT_NAP_US,
                    (sig >= 0) ? (1UL << sig) : 0);
    }

    if (sig >= 0)
    {
        /* Stop the handler signalling a bit that is about to be freed */
        Disable();
        sd->waiter = NULL;
        Enable();
        FreeSignal(sig);
    }
    return ret;
}

/* Room for another job: the one binner is free and the render queue has
 * a slot. It no longer depends on any render having finished, which is
 * what used to make a submit block for a whole render. */
static BOOL v3d_slot_free(struct V3DData *sd)
{
    return !sd->bin_running && sd->rcl_count < V3D_RCL_QUEUE;
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

    /* Queue the render before starting the bin: the flush can land in
     * the handler the instant the bin is kicked, and the entry has to be
     * there for it to be picked up. */
    Disable();
    if (rcl_start != rcl_end)
    {
        UBYTE tail = (sd->rcl_head + sd->rcl_count) % V3D_RCL_QUEUE;

        sd->rcl_q[tail].start = rcl_start;
        sd->rcl_q[tail].end   = rcl_end;
        sd->rcl_q[tail].seqno = seqno;
        sd->rcl_count++;
        sd->bin_only = FALSE;
        /* No bin of its own: nothing will ever flush for it, so date it
         * as already binned or the queue would stall on it. */
        if (bcl_start == bcl_end)
            sd->bin_flushed_seqno = seqno;
    }
    else
    {
        sd->bin_only = TRUE;
        if (bcl_start == bcl_end)
            sd->finished_seqno = seqno;      /* empty submission */
    }
    Enable();

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
        sd->oom_pending = FALSE;
        sd->oom_handed = FALSE;
#if V3D_PROFILE
        sd->prof_bin_kick = v3d_now_us();
#endif
        v3d_core_wr(sd, V3D_CLE_CT0QBA, bcl_start);
        v3d_core_wr(sd, V3D_CLE_CT0QEA, bcl_end);
        D(bug("[V3D] bin submit %u: QBA=%08x QEA=%08x\n", (unsigned)seqno,
              bcl_start, bcl_end));
    }

    /* An RCL with no BCL (or a bin that flushed instantly) can go now */
    v3d_service(sd);

#if V3D_PROFILE
    if (sd->prof_rep_jobs)
    {
        V3D_PROF("[V3DProf] gpu: %lu jobs bin=%luus render=%luus "
                 "oom=%lu spill=%lu\n",
                 (unsigned long)sd->prof_rep_jobs,
                 (unsigned long)sd->prof_rep_bin,
                 (unsigned long)sd->prof_rep_render,
                 (unsigned long)sd->oom_events,
                 (unsigned long)sd->spill_events);
        sd->prof_rep_jobs = 0;
    }
#endif

    ReleaseSemaphore(&sd->job_lock);
    return TRUE;
}

static BOOL v3d_seqno_reached(struct V3DData *sd)
{
    return sd->finished_seqno >= sd->wait_seqno;
}

/* Wait for ONE submission, not for the pipeline: the frame being
 * presented retired long ago, while the one just submitted keeps
 * rendering behind it. Waiting for the pipeline serialises CPU and GPU. */
void v3d_hw_wait_seqno(struct V3DData *sd, ULONG seqno)
{
    ObtainSemaphore(&sd->job_lock);
    if (sd->powered && seqno && sd->finished_seqno < seqno)
    {
        sd->wait_seqno = seqno;
        v3d_wait_for(sd, v3d_seqno_reached);
    }
    ReleaseSemaphore(&sd->job_lock);
}

void v3d_wait_idle(struct V3DData *sd)
{
#if V3D_PROFILE
    /* How long the CPU actually spends blocked on the GPU, and the worst
     * single case - an average alone hides one pathological frame. */
    static ULONG n, acc, worst;
    ULONG t0 = V3D_NOW_US();
#endif

    ObtainSemaphore(&sd->job_lock);
    if (sd->powered)
    {
        if (v3d_wait_for(sd, v3d_pipeline_idle))
            v3d_report_once(sd);
    }
    ReleaseSemaphore(&sd->job_lock);

#if V3D_PROFILE
    {
        ULONG d = V3D_NOW_US() - t0;

        acc += d;
        if (d > worst)
            worst = d;
        if (++n >= V3D_PROF_PERIOD)
        {
            V3D_PROF("[V3DProf] wait_idle: %lu calls avg=%luus max=%luus\n",
                     (unsigned long)n, (unsigned long)(acc / n),
                     (unsigned long)worst);
            n = acc = worst = 0;
        }
    }
#endif
}
