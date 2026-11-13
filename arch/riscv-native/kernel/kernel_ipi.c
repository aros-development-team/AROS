/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Inter-processor interrupts for native RISC-V SMP kernels.

    Delivery: a sender ORs its message bits into the target's pending
    word and raises the target's supervisor software interrupt through
    the platform (RVI_SendIPI - an SBI call on riscv64-opensbi). The
    software interrupt carries no payload, so the pending word is the
    message; the handler clears sip.SSIP first and then takes the word,
    so a message posted while it runs raises a fresh interrupt instead
    of being lost.

    Hook calls (IPI_CALL_HOOK) queue on the target, following the
    aarch64-native model: entries come from a per-target pool so that
    Signal(), which may be called from interrupts, never has to
    allocate; claim and commit are split so exec can check the target
    task's lifetime atomically with the enqueue (signal.c).
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <aros/types/spinlock_s.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/* Non-atomic flag updates: the scheduling flags are per-CPU TLS */
#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#include "kernel_riscv.h"
#include "kernel_spinlock.h"

#if defined(__AROSEXEC_SMP__)

#include "kernel_ipi.h"

#define D(x)

struct CallIPIEntry
{
    struct MinNode      cie_Node;
    struct IPIHook      cie_IPIH;   /* h_Entry + ih_Args - passed as A0 to the hook */
    volatile ULONG      *cie_Done;  /* sync calls: count of CPUs still to run it */
};

#define IPI_CALL_POOL_PER_CPU   64

struct CPUIPIData
{
    volatile ULONG      cid_Pending;    /* IPI_* bits posted to this CPU   */
    spinlock_t          cid_Lock;       /* guards both lists               */
    struct MinList      cid_Free;
    struct MinList      cid_Queue;
    /* What the drain is running now, so a canceller can wait out a call
       it can no longer find queued. Single writer: the CPU itself. */
    volatile APTR       cid_ExecFunc;
    volatile IPTR       cid_ExecArg1;
};

static struct CPUIPIData *ipi_data[RISCV_MAXCPUS];

extern void krnRVCPUOffline(uint32_t cpu);

/* Interrupts masked: a preempted holder would deadlock a Disable()d waiter */
static inline unsigned int ipiq_lock(struct CPUIPIData *cid)
{
    unsigned int prev = krnRVIntrDisable();

    while (!krnRVSpinTryWrite(&cid->cid_Lock))
    {
        while (cid->cid_Lock.lock != SPINLOCK_UNLOCKED)
            krnRVPause();
    }
    return prev;
}

static inline void ipiq_unlock(struct CPUIPIData *cid, unsigned int prev)
{
    asm volatile("amoswap.w.rl zero, zero, (%0)" :: "r"(&cid->cid_Lock.lock) : "memory");
    krnRVIntrRestore(prev);
}

static inline BOOL core_IntrMasked(void)
{
    unsigned long sstatus;

    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    return !(sstatus & 2);
}

/*
 * Set up the IPI state of every CPU that may come online. Called once
 * by the platform, after memory is available and before any secondary
 * hart is started.
 */
void core_IPIInit(void)
{
    struct ExecBase *SysBase = TLS_GET(SysBase);
    uint32_t cpu;
    int i;

    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
    {
        struct CPUIPIData *cid;
        struct CallIPIEntry *pool;

        if (ipi_data[cpu])
            continue;

        cid = AllocMem(sizeof(struct CPUIPIData) +
                       IPI_CALL_POOL_PER_CPU * sizeof(struct CallIPIEntry),
                       MEMF_PUBLIC | MEMF_CLEAR);
        if (!cid)
            break;

        NewMinList(&cid->cid_Free);
        NewMinList(&cid->cid_Queue);
        pool = (struct CallIPIEntry *)(cid + 1);
        for (i = 0; i < IPI_CALL_POOL_PER_CPU; i++)
            ADDTAIL((struct List *)&cid->cid_Free, (struct Node *)&pool[i].cie_Node);

        __atomic_store_n(&ipi_data[cpu], cid, __ATOMIC_RELEASE);
    }
}

void core_SendIPI(ULONG msg, const ULONG *mask)
{
    ULONG targets[RISCV_CPUMASK_WORDS];
    BOOL any = FALSE;
    uint32_t cpu;

    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
    {
        ULONG bit = 1UL << (cpu % 32);

        if (!(cpu % 32))
            targets[cpu / 32] = 0;

        if (!(mask[cpu / 32] & bit) || !(__riscv_cpu_online[cpu / 32] & bit) ||
            !ipi_data[cpu])
            continue;

        __atomic_fetch_or(&ipi_data[cpu]->cid_Pending, msg, __ATOMIC_RELEASE);
        targets[cpu / 32] |= bit;
        any = TRUE;
    }

    /* The posts must be visible before the interrupt that reports them */
    asm volatile("fence rw, rw" ::: "memory");

    if (any && __riscv_arosintern.RVI_SendIPI)
        __riscv_arosintern.RVI_SendIPI(targets);
}

static void core_HandleCallHookIPI(uint32_t cpu);

/*
 * Claim a free entry for the target, spinning rather than dropping the
 * call - a dropped IPI is a lost wakeup that hangs a task forever.
 *
 * With interrupts masked this CPU can not drain its own inbound queue,
 * so two such CPUs signalling each other with empty pools would wait on
 * each other. Break that by draining our own queue inline while we spin.
 *
 * Call with NO task spinlock held: the drain runs hooks that take them.
 */
struct CallIPIEntry *core_ClaimCallIPI(int cpu)
{
    struct CPUIPIData *cid = ipi_data[cpu];
    uint32_t self = krnRVCPUNumber();
    struct CallIPIEntry *cie;

    for (;;)
    {
        unsigned int prev = ipiq_lock(cid);

        cie = (struct CallIPIEntry *)REMHEAD((struct List *)&cid->cid_Free);
        ipiq_unlock(cid, prev);

        if (cie)
        {
            cie->cie_Done = NULL;
            return cie;
        }

        if (core_IntrMasked())
            core_HandleCallHookIPI(self);
        else
            krnRVPause();
    }
}

void core_CommitCallIPI(struct CallIPIEntry *cie, int cpu,
                        struct Hook *hook, int nargs, IPTR *args)
{
    struct CPUIPIData *cid = ipi_data[cpu];
    ULONG mask[RISCV_CPUMASK_WORDS] = { 0 };
    unsigned int prev;
    int i;

    if (nargs > IPI_CALL_HOOK_MAX_ARGS)
        nargs = IPI_CALL_HOOK_MAX_ARGS;

    cie->cie_IPIH.ih_Hook = *hook;
    for (i = 0; i < nargs; i++)
        cie->cie_IPIH.ih_Args[i] = args[i];

    prev = ipiq_lock(cid);
    ADDTAIL((struct List *)&cid->cid_Queue, (struct Node *)&cie->cie_Node);
    ipiq_unlock(cid, prev);

    mask[cpu / 32] = 1UL << (cpu % 32);
    core_SendIPI(IPI_CALL_HOOK, mask);
}

void core_AbortCallIPI(struct CallIPIEntry *cie, int cpu)
{
    struct CPUIPIData *cid = ipi_data[cpu];
    unsigned int prev = ipiq_lock(cid);

    ADDTAIL((struct List *)&cid->cid_Free, (struct Node *)&cie->cie_Node);
    ipiq_unlock(cid, prev);
}

/*
 * Cancel queued calls matching (h_Entry, ih_Args[1]) everywhere, then
 * wait out one already executing. On return no CPU will touch the
 * object - provided the caller has made new commits impossible.
 */
void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg)
{
    uint32_t cpu;

    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
    {
        struct CPUIPIData *cid = ipi_data[cpu];
        struct MinNode *node, *next;
        unsigned int prev;

        if (!cid)
            continue;

        prev = ipiq_lock(cid);
        for (node = cid->cid_Queue.mlh_Head; (next = node->mln_Succ) != NULL; node = next)
        {
            struct CallIPIEntry *cie = (struct CallIPIEntry *)node;

            /* A sync caller is waiting on its entries; leave them be */
            if (!cie->cie_Done &&
                (APTR)cie->cie_IPIH.ih_Hook.h_Entry == hookEntry &&
                cie->cie_IPIH.ih_Args[1] == matchArg)
            {
                REMOVE((struct Node *)node);
                ADDTAIL((struct List *)&cid->cid_Free, (struct Node *)node);
            }
        }
        ipiq_unlock(cid, prev);

        while (cid->cid_ExecFunc == hookEntry && cid->cid_ExecArg1 == matchArg)
            krnRVPause();
    }
}

/*
 * Run hook on every CPU in cpu_mask. With async = 0 the call returns
 * once every target has run it (the caller's CPU included, which runs
 * it directly).
 */
int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async,
                   int nargs, IPTR *args, APTR _KB)
{
    ULONG mask[RISCV_CPUMASK_WORDS];
    uint32_t self = krnRVCPUNumber();
    volatile ULONG done = 0;
    BOOL runlocal;
    uint32_t cpu;

    (void)_KB;

    if (!hook || !cpu_mask || (nargs > IPI_CALL_HOOK_MAX_ARGS))
        return 0;

    krnRVResolveMask(cpu_mask, self, mask);
    runlocal = (mask[self / 32] & (1UL << (self % 32))) != 0;
    mask[self / 32] &= ~(1UL << (self % 32));

    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
    {
        struct CallIPIEntry *cie;

        if (!(mask[cpu / 32] & (1UL << (cpu % 32))) || !ipi_data[cpu])
            continue;

        cie = core_ClaimCallIPI(cpu);
        /* Before the commit - the target may run it straight away */
        if (!async)
        {
            __atomic_fetch_add(&done, 1, __ATOMIC_RELAXED);
            cie->cie_Done = &done;
        }
        core_CommitCallIPI(cie, cpu, hook, nargs, args);
    }

    if (runlocal)
    {
        struct IPIHook ih;
        int i;

        ih.ih_Hook = *hook;
        for (i = 0; i < nargs; i++)
            ih.ih_Args[i] = args[i];

        AROS_UFC3NR(void, ih.ih_Hook.h_Entry,
            AROS_UFCA(struct IPIHook *, &ih, A0),
            AROS_UFCA(APTR, NULL, A2),
            AROS_UFCA(APTR, NULL, A1));
    }

    /* Keep our own queue moving while we wait, or two CPUs calling each
       other synchronously with interrupts masked would never finish */
    while (__atomic_load_n(&done, __ATOMIC_ACQUIRE))
    {
        if (core_IntrMasked())
            core_HandleCallHookIPI(self);
        else
            krnRVPause();
    }

    return 1;
}

static void core_HandleCallHookIPI(uint32_t cpu)
{
    struct CPUIPIData *cid = ipi_data[cpu];

    if (!cid)
        return;

    /* Block dispatch across the drain: the hooks call Enable()/Reschedule() */
    EXEC_BLOCK_DISPATCH_INC;

    for (;;)
    {
        struct CallIPIEntry *cie;
        volatile ULONG *done;
        unsigned int prev = ipiq_lock(cid);

        cie = (struct CallIPIEntry *)REMHEAD((struct List *)&cid->cid_Queue);
        if (cie)
        {
            /* Once dequeued the call is invisible to the canceller's
             * queue scan, so it waits on this instead. */
            cid->cid_ExecFunc = (APTR)cie->cie_IPIH.ih_Hook.h_Entry;
            cid->cid_ExecArg1 = cie->cie_IPIH.ih_Args[1];
        }
        ipiq_unlock(cid, prev);

        if (!cie)
            break;

        /* signal_hook's signature: A0 = IPIHook*, A1/A2 unused */
        AROS_UFC3NR(void, cie->cie_IPIH.ih_Hook.h_Entry,
            AROS_UFCA(struct IPIHook *, &cie->cie_IPIH, A0),
            AROS_UFCA(APTR, NULL, A2),
            AROS_UFCA(APTR, NULL, A1));

        /* Hook side effects visible before a canceller is released */
        asm volatile("fence rw, rw" ::: "memory");
        cid->cid_ExecFunc = NULL;

        done = cie->cie_Done;
        cie->cie_Done = NULL;

        prev = ipiq_lock(cid);
        ADDTAIL((struct List *)&cid->cid_Free, (struct Node *)&cie->cie_Node);
        ipiq_unlock(cid, prev);

        if (done)
            __atomic_fetch_sub(done, 1, __ATOMIC_RELEASE);
    }

    EXEC_BLOCK_DISPATCH_DEC;
}

/* Supervisor software interrupt: called from the trap dispatch */
void core_IPIHandle(void)
{
    uint32_t cpu = krnRVCPUNumber();
    struct CPUIPIData *cid = ipi_data[cpu];
    ULONG msg;

    /* Acknowledge first, so a post that lands from here on interrupts again */
    asm volatile("csrc sip, %0" :: "r"(2UL) : "memory");

    if (!cid)
        return;

    msg = __atomic_exchange_n(&cid->cid_Pending, 0, __ATOMIC_ACQUIRE);

    D(bug("[Kernel:IPI] CPU%02u: msg %08x\n", cpu, msg));

    if (msg & IPI_SCHEDULE)
    {
        /* Expire the quantum too: core_Schedule() keeps a task that has
           time left, so an equal-priority task would never get an idle
           CPU otherwise */
        FLAG_SCHEDQUANTUM_SET;
        FLAG_SCHEDSWITCH_SET;
    }
    if (msg & IPI_CALL_HOOK)
        core_HandleCallHookIPI(cpu);
    if ((msg & IPI_REBOOT) && cpu && __riscv_arosintern.RVI_StopCPU)
    {
        /* The kernel is about to be restarted in place (krnWarmKick):
           leave the online set and stop for good. Never on CPU 0. */
        krnRVCPUOffline(cpu);
        __riscv_arosintern.RVI_StopCPU();
    }
}

#endif /* __AROSEXEC_SMP__ */
