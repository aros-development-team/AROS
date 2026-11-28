/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Inter-processor interrupts for native RISC-V SMP kernels.

    Delivery: a sender ORs its message bits into the target's pending
    word and, if they were not pending there already, raises the target's
    software interrupt through the platform (RVI_SendIPI - an SBI call on
    riscv64-sbi). The software interrupt carries no payload, so the
    pending word is the message; the handler clears its pending bit first
    and then takes the word, so a message posted while it runs raises a
    fresh interrupt instead of being lost.

    The pending words, the hook calls (IPI_CALL_HOOK) and their queues
    are the shared ones in rom/kernel/kernel_ipicall.c.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <aros/types/spinlock_s.h>
#include <asm/cpu.h>

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

extern void krnRVCPUOffline(uint32_t cpu);

static inline BOOL core_IntrMasked(void)
{
    return !(csr_read(RV_CSR_STATUS) & RV_STATUS_IE);
}

/*
 * Set up the request words and call queues of every CPU that may come
 * online (rom/kernel/kernel_ipicall.c). Called once by the platform, after
 * memory is available and before any secondary hart is started.
 */
void core_IPIInit(void)
{
    krnIPICallInit(RISCV_MAXCPUS);
}

/*
 * What rom/kernel/kernel_ipicall.c needs from this port: how a hart is
 * interrupted. The software interrupt carries no payload; the request bits
 * are in the hart's pending word, which the shared code keeps.
 */
void krnIPIPortRaise(ULONG cpu, ULONG bits)
{
    ULONG targets[RISCV_CPUMASK_WORDS];
    ULONG i;

    (void)bits;

    for (i = 0; i < RISCV_CPUMASK_WORDS; i++)
        targets[i] = 0;
    targets[cpu / 32] = 1UL << (cpu % 32);

    /* The request must be visible before the interrupt that reports it */
    asm volatile("fence rw, rw" ::: "memory");

    if (__riscv_arosintern.RVI_SendIPI)
        __riscv_arosintern.RVI_SendIPI(targets);
}

/* A hart with interrupts masked cannot take its IPI: a sender that waits runs its own calls */
BOOL krnIPIPortCanServe(void)
{
    return core_IntrMasked();
}

static void core_HandleCallHookIPI(void)
{
    /* Block dispatch across the drain: the hooks call Enable()/Reschedule() */
    EXEC_BLOCK_DISPATCH_INC;
    krnIPICallDrain();
    EXEC_BLOCK_DISPATCH_DEC;
}

void krnIPIPortServe(void)
{
    unsigned int prev = rv_intr_disable();

    core_HandleCallHookIPI();
    rv_intr_restore(prev);
}

/*
 * Post msg to every online CPU in the (resolved) mask. A CPU that has the
 * message pending already is not interrupted again.
 */
void core_SendIPI(ULONG msg, const ULONG *mask)
{
    uint32_t cpu;

    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
    {
        ULONG bit = 1UL << (cpu % 32);

        if (!(mask[cpu / 32] & bit) || !(__riscv_cpu_online[cpu / 32] & bit))
            continue;

        krnIPIPost(cpu, msg);
    }
}

/* Drop the queued calls of hookEntry whose second argument is matchArg, and wait out one that is running */
void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg)
{
    krnIPICallCancel(hookEntry, matchArg);
}

/*
 * Run hook on every CPU in cpu_mask. With async = 0 the call returns
 * once every target has run it. The calling CPU is interrupted like any
 * other if it is in the mask.
 */
int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async,
                   int nargs, IPTR *args, APTR _KB)
{
    ULONG mask[RISCV_CPUMASK_WORDS];
    uint32_t cpu;

    (void)_KB;

    if (!hook || !cpu_mask || (nargs > IPI_CALL_HOOK_MAX_ARGS))
        return 0;

    krnRVResolveMask(cpu_mask, krnRVCPUNumber(), mask);

    /* Only CPUs that are online */
    for (cpu = 0; cpu < RISCV_CPUMASK_WORDS; cpu++)
        mask[cpu] &= __riscv_cpu_online[cpu];

    krnIPICall(hook, mask, RISCV_CPUMASK_WORDS, async ? TRUE : FALSE, nargs, args);

    return 1;
}

/* Software interrupt (SSI): called from the trap dispatch */
void core_IPIHandle(void)
{
    uint32_t cpu = krnRVCPUNumber();
    ULONG msg;

    /* Acknowledge first, so a post that lands from here on interrupts again */
    if (__riscv_arosintern.RVI_AckIPI)
        __riscv_arosintern.RVI_AckIPI();

    msg = krnIPIAck(~0UL);

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
        core_HandleCallHookIPI();

    if ((msg & IPI_REBOOT) && cpu && __riscv_arosintern.RVI_StopCPU)
    {
        /* The kernel is about to be restarted in place (krnWarmKick):
           leave the online set and stop for good. Never on CPU 0. */
        krnRVCPUOffline(cpu);
        __riscv_arosintern.RVI_StopCPU();
    }
}

#endif /* __AROSEXEC_SMP__ */
