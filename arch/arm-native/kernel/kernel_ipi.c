/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/types/spinlock_s.h>
#include <aros/arm/cpucontext.h>

#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include "kernel_base.h"

/* Pull in exec_platform.h ahead of etask.h so AROS_NO_ATOMIC_OPERATIONS
 * selects the non-atomic FLAG_SCHEDSWITCH_SET expansion (atomics aren't
 * needed for per-CPU TLS state and AROS_ATOMIC_OR isn't declared here). */
#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#include "etask.h"

#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_scheduler.h"

#if defined(__AROSEXEC_SMP__)

#undef D
#define D(x)

#include "kernel_ipi.h"

/*
 * Per-target queue of pending IPIHook calls. Sender pops a CallIPIEntry
 * from the target's free pool, fills it in, pushes it on the target's
 * busy queue, and sends an IPI_CALL_HOOK message. The receiver drains
 * its own queue from handle_ipi(), calls each hook, and returns the
 * entry to the free pool.
 *
 * Entries come from a per-target static pool sized for the worst
 * concurrent burst. AllocMem cannot be used here: Signal() is
 * documented as IRQ-callable and reaches core_DoCallIPI on cross-CPU
 * wakeups; AllocMem would recurse on mh_SpinLock if an interrupted
 * allocator on this CPU held it.
 *
 * raspi has at most 4 cores. Async-only - signal.c is the sole caller
 * and uses async; the sync path is more involved (would need a lock
 * dance with the sender waiting for completion) and is left for if
 * a real consumer appears.
 */
struct CallIPIEntry
{
    struct MinNode cie_Node;
    struct IPIHook cie_IPIH;        /* h_Entry + ih_Args - passed as A0 to the hook */
};

#define IPI_CALL_POOL_PER_CPU   128

static struct CallIPIEntry ipi_call_pool[4][IPI_CALL_POOL_PER_CPU];
static struct MinList      ipi_call_free[4];
static struct MinList      ipi_call_queue[4];
static spinlock_t          ipi_call_queue_lock[4];   /* guards both lists for this target */
static BOOL                ipi_call_inited = FALSE;

/*
 * What the drain loop is executing right now, published per CPU so
 * core_CancelCallIPIs can wait out a call it can no longer find on the
 * queue. Single writer (the owning CPU's FIQ drain); cross-CPU readers.
 */
static volatile APTR       ipi_call_exec_func[4];
static volatile IPTR       ipi_call_exec_arg1[4];

static void core_HandleCallHookIPI(int cpu);

void core_IPIInit(void)
{
    int cpu, i;
    if (ipi_call_inited)
        return;
    for (cpu = 0; cpu < 4; cpu++)
    {
        NewMinList(&ipi_call_free[cpu]);
        NewMinList(&ipi_call_queue[cpu]);
        for (i = 0; i < IPI_CALL_POOL_PER_CPU; i++)
            ADDTAIL((struct List *)&ipi_call_free[cpu],
                    (struct Node *)&ipi_call_pool[cpu][i].cie_Node);
        /* spinlock_t zero-initialised at link time == SPINLOCK_UNLOCKED */
    }
    ipi_call_inited = TRUE;
}

/*
 * Claim a free pool entry for the given target. If the pool is
 * momentarily exhausted, the target already has IPI_CALL_HOOK
 * entries queued (an IPI was sent for each) and drains them back to
 * its free list from its FIQ handler. Spin until one frees rather
 * than dropping the call: the hook is signal_hook, so a dropped IPI
 * is a lost wakeup that hangs the waiting task forever.
 *
 * The claim only runs in task/IRQ context (signal_hook does its work
 * inline and never calls Signal(), so the FIQ path never re-enters
 * here). With FIQ unmasked the target always drains promptly. The one
 * cycle left is Signal() under Disable(): FIQ is masked here, so this
 * CPU cannot drain its own inbound queue - two Disable()-d CPUs
 * cross-signalling with exhausted pools would wait on each other's
 * drains forever. Break it by draining our own queue inline while we
 * spin: that refills OUR pool, unblocking the peer, whose Enable()
 * eventually refills the pool we are waiting for. Safe precisely
 * because FIQ is masked - the FIQ drain cannot run concurrently, so
 * the executing-slot publication keeps its single writer per CPU.
 *
 * Corollary: call this with NO task spinlock held. The drains that
 * refill pools run hooks that take task spinlocks, so spinning here
 * while holding one could deadlock.
 */
struct CallIPIEntry *core_ClaimCallIPI(int cpu)
{
    struct CallIPIEntry *cie;
    int srcCpu = GetCPUNumber();

    for (;;)
    {
        EXEC_SPINLOCK_LOCK(&ipi_call_queue_lock[cpu], NULL, SPINLOCK_MODE_WRITE);
        cie = (struct CallIPIEntry *)REMHEAD((struct List *)&ipi_call_free[cpu]);
        EXEC_SPINLOCK_UNLOCK(&ipi_call_queue_lock[cpu]);

        if (cie)
            return cie;

        if (IDNESTCOUNT_GET >= 0)
            core_HandleCallHookIPI(srcCpu);
    }
}

void core_CommitCallIPI(struct CallIPIEntry *cie, int cpu,
                        struct Hook *hook, int nargs, IPTR *args)
{
    int srcCpu = GetCPUNumber();
    int i;

    if (nargs > IPI_CALL_HOOK_MAX_ARGS)
        nargs = IPI_CALL_HOOK_MAX_ARGS;

    cie->cie_IPIH.ih_Hook = *hook;
    for (i = 0; i < nargs; i++)
        cie->cie_IPIH.ih_Args[i] = args[i];

    EXEC_SPINLOCK_LOCK(&ipi_call_queue_lock[cpu], NULL, SPINLOCK_MODE_WRITE);
    ADDTAIL((struct List *)&ipi_call_queue[cpu], (struct Node *)&cie->cie_Node);
    EXEC_SPINLOCK_UNLOCK(&ipi_call_queue_lock[cpu]);

    if (__arm_arosintern.ARMI_SendIPI)
    {
        __arm_arosintern.ARMI_SendIPI(
            (IPI_CALL_HOOK & 0x0fffffff) | (srcCpu << 28),
            0, 1U << cpu);
    }
}

void core_AbortCallIPI(struct CallIPIEntry *cie, int cpu)
{
    EXEC_SPINLOCK_LOCK(&ipi_call_queue_lock[cpu], NULL, SPINLOCK_MODE_WRITE);
    ADDTAIL((struct List *)&ipi_call_free[cpu], (struct Node *)&cie->cie_Node);
    EXEC_SPINLOCK_UNLOCK(&ipi_call_queue_lock[cpu]);
}

/*
 * Cancel all queued calls matching (h_Entry, ih_Args[1]) on every CPU,
 * then wait out a matching call that is executing right now. On return
 * no CPU holds or will touch the object ih_Args[1] points at, PROVIDED
 * the caller has already made new commits impossible (signal.c re-checks
 * the target's tc_State under tc_SpinLock before core_CommitCallIPI).
 *
 * Must run with FIQ masked (Disable): we take our own CPU's queue lock,
 * which the local FIQ drain also takes. The executing-call wait only
 * ever waits on OTHER CPUs (our own drain cannot be mid-hook while we
 * run), and their hooks are short and non-blocking, so the wait is
 * bounded even under Disable.
 */
void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg)
{
    int cpu;

    if (!ipi_call_inited)
        return;

    for (cpu = 0; cpu < 4; cpu++)
    {
        struct MinNode *node, *next;

        EXEC_SPINLOCK_LOCK(&ipi_call_queue_lock[cpu], NULL, SPINLOCK_MODE_WRITE);
        for (node = ipi_call_queue[cpu].mlh_Head; (next = node->mln_Succ) != NULL; node = next)
        {
            struct CallIPIEntry *cie = (struct CallIPIEntry *)node;

            if ((APTR)cie->cie_IPIH.ih_Hook.h_Entry == hookEntry &&
                cie->cie_IPIH.ih_Args[1] == matchArg)
            {
                REMOVE((struct Node *)node);
                ADDTAIL((struct List *)&ipi_call_free[cpu], (struct Node *)node);
            }
        }
        EXEC_SPINLOCK_UNLOCK(&ipi_call_queue_lock[cpu]);

        while (ipi_call_exec_func[cpu] == hookEntry &&
               ipi_call_exec_arg1[cpu] == matchArg)
        {
            EXEC_MEMORY_BARRIER();
        }
    }
}

int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async,
                   int nargs, IPTR *args, APTR _KB)
{
    int cpu;
    int srcCpu = GetCPUNumber();
    uint32_t mask;

    (void)_KB;

    if (!hook || !cpu_mask)
        return 0;
    if (nargs > IPI_CALL_HOOK_MAX_ARGS)
        return 0;
    /* Sync mode not yet supported on arm-native */
    if (!async)
    {
        D(bug("[Kernel:IPI] %s: sync mode not implemented\n", __PRETTY_FUNCTION__));
        return 0;
    }

    /*
     * TASKAFFINITY_ANY / TASKAFFINITY_ALL_BUT_SELF are sentinels, not
     * buffer pointers - resolve them to bitmaps before dereference.
     */
    if ((IPTR)cpu_mask == TASKAFFINITY_ANY)
        mask = 0xf;
    else if ((IPTR)cpu_mask == TASKAFFINITY_ALL_BUT_SELF)
        mask = 0xf & ~(1U << srcCpu);
    else
        mask = *(uint32_t *)cpu_mask;

    for (cpu = 0; cpu < 4; cpu++)
    {
        struct CallIPIEntry *cie;

        if (!(mask & (1U << cpu)))
            continue;

        cie = core_ClaimCallIPI(cpu);
        core_CommitCallIPI(cie, cpu, hook, nargs, args);
    }

    return 1;
}

static void core_HandleCallHookIPI(int cpu)
{
    /*
     * Block dispatch across the whole drain. The hooks here (signal_hook ->
     * Signal()) call Enable()/Reschedule(), which would otherwise switch
     * tasks *now* - from inside this FIQ handler, on the nested SVC stack -
     * corrupting the interrupted context. With dispatch blocked, Signal()
     * only sets FLAG_SCHEDSWITCH; the switch is taken cleanly by
     * core_ExitInterrupt when this FIQ returns to a task context. (The
     * scheduler-list lock re-entrancy this used to expose is handled by the
     * EXEC_FIQ_DISABLE masking in the scheduler critical sections.)
     */
    EXEC_BLOCK_DISPATCH_INC;

    for (;;)
    {
        struct CallIPIEntry *cie;

        EXEC_SPINLOCK_LOCK(&ipi_call_queue_lock[cpu], NULL, SPINLOCK_MODE_WRITE);
        cie = (struct CallIPIEntry *)REMHEAD((struct List *)&ipi_call_queue[cpu]);
        if (cie)
        {
            /*
             * Publish what is about to run: once dequeued, the call is
             * invisible to core_CancelCallIPIs' queue scan, so a canceller
             * waits on this instead. Published under the queue lock - the
             * unlock's barrier orders it before the hook runs.
             */
            ipi_call_exec_func[cpu] = (APTR)cie->cie_IPIH.ih_Hook.h_Entry;
            ipi_call_exec_arg1[cpu] = cie->cie_IPIH.ih_Args[1];
        }
        EXEC_SPINLOCK_UNLOCK(&ipi_call_queue_lock[cpu]);

        if (!cie)
            break;

        /*
         * Hook signature matches signal_hook: A0 = IPIHook*, A2/A1 unused.
         * cie_IPIH starts at the same offset as a struct IPIHook so passing
         * &cie_IPIH gives the hook direct access to its ih_Args[].
         */
        AROS_UFC3NR(void, cie->cie_IPIH.ih_Hook.h_Entry,
            AROS_UFCA(struct IPIHook *, &cie->cie_IPIH, A0),
            AROS_UFCA(APTR, NULL, A2),
            AROS_UFCA(APTR, NULL, A1));

        /*
         * All hook side effects must be visible before a waiting canceller
         * is released to free the object ih_Args[1] points at.
         */
        EXEC_MEMORY_BARRIER();
        ipi_call_exec_func[cpu] = NULL;

        EXEC_SPINLOCK_LOCK(&ipi_call_queue_lock[cpu], NULL, SPINLOCK_MODE_WRITE);
        ADDTAIL((struct List *)&ipi_call_free[cpu], (struct Node *)&cie->cie_Node);
        EXEC_SPINLOCK_UNLOCK(&ipi_call_queue_lock[cpu]);
    }

    EXEC_BLOCK_DISPATCH_DEC;
}

void handle_ipi(uint32_t ipi, uint32_t ipi_data)
{
    int cpu = GetCPUNumber();
    uint32_t ipi_src = (ipi >> 28) & 0xF;
    uint32_t ipi_msg = ipi & ~(0xF << 28);

    D(bug("[Kernel:IPI] %s: Core #%02d IPI Msg %08x:%08x from Core #%02d\n",
        __PRETTY_FUNCTION__, cpu, ipi_msg,  ipi_data, ipi_src));

    /*
     * IPI messages are bit flags. The BCM2836 mailbox SET register
     * OR-coalesces concurrent writes from the same sender, so a single
     * delivery may carry multiple message types. Test each bit
     * independently rather than switching on the whole value.
     */
    if (ipi_msg & IPI_CAUSE)
        D(bug("[Kernel:IPI] IPI_CAUSE:\n"));
    if (ipi_msg & IPI_DISPATCH)
        D(bug("[Kernel:IPI] IPI_DISPATCH:\n"));
    if (ipi_msg & IPI_SWITCH)
        D(bug("[Kernel:IPI] IPI_SWITCH:\n"));
    if (ipi_msg & IPI_SCHEDULE)
    {
        /*
         * Flag the local CPU for reschedule. The next preemption
         * opportunity (Permit/Enable, next timer tick on core 0,
         * or completion of the current FIQ return) will pick this
         * up. Idle CPUs already wake from WFI on FIQ delivery
         * and re-check core_Dispatch in their idle loop, so no
         * extra work is needed for the idle-wake case.
         */
        D(bug("[Kernel:IPI] IPI_SCHEDULE:\n"));
        /*
         * Also expire the quantum: core_Schedule() only keeps the running
         * task if the best ready candidate has <= priority AND the quantum
         * is still unused. Secondary cores have no local timer to expire it,
         * so without this an equal-priority task IPI'd to an idle core would
         * never preempt the idle context.
         */
        FLAG_SCHEDQUANTUM_SET;
        FLAG_SCHEDSWITCH_SET;
    }
    if (ipi_msg & IPI_CALL_HOOK)
    {
        D(bug("[Kernel:IPI] IPI_CALL_HOOK:\n"));
        core_HandleCallHookIPI(cpu);
    }
    if (ipi_msg & IPI_CLI)
        D(bug("[Kernel:IPI] IPI_CLI:\n"));
    if (ipi_msg & IPI_STI)
        D(bug("[Kernel:IPI] IPI_STI:\n"));
    if (ipi_msg & IPI_REBOOT)
        D(bug("[Kernel:IPI] IPI_REBOOT:\n"));
    if (ipi_msg & IPI_ADDTASK)
        D(bug("[Kernel:IPI] IPI_ADDTASK:\n"));
    if (ipi_msg & IPI_REMTASK)
        D(bug("[Kernel:IPI] IPI_REMTASK:\n"));
}
#endif
