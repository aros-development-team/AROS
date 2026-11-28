/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/types/spinlock_s.h>
#include <aros/aarch64/cpucontext.h>

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
 * The requests and the hook calls between cores, and their queues, are the
 * shared ones in rom/kernel/kernel_ipicall.c. Here is only how a core is
 * interrupted, and what it does with the messages it finds.
 */
void core_IPIInit(void)
{
    krnIPICallInit(KRN_IPICALL_MAXCPUS);
}

/* The IPI is an FIQ or an IRQ, depending on the board: with either one
 * masked this CPU may be unable to take it */
static inline int core_IPIMasked(void)
{
    uint64_t daif;

    asm volatile("mrs %0, daif" : "=r"(daif));

    return (daif & ((1 << 7) | (1 << 6))) != 0;
}

/* A CPU that cannot take its IPI: a sender that waits runs its own calls */
BOOL krnIPIPortCanServe(void)
{
    return (IDNESTCOUNT_GET >= 0) || core_IPIMasked();
}

/* Interrupt one core for the requests that have just become pending */
void krnIPIPortRaise(ULONG cpu, ULONG bits)
{
    if (__arm_arosintern.ARMI_SendIPI)
    {
        __arm_arosintern.ARMI_SendIPI(
            (bits & 0x0fffffff) | (GetCPUNumber() << 28),
            0, 1U << cpu);
    }
}

static void core_HandleCallHookIPI(void)
{
    /* No task switch while the hooks run: a Reschedule() in a hook only
     * marks the switch as pending. It is carried out on the way out of the
     * interrupt, or by the Enable() of a task that ran the calls itself. */
    EXEC_BLOCK_DISPATCH_INC;
    krnIPICallDrain();
    EXEC_BLOCK_DISPATCH_DEC;
}

void krnIPIPortServe(void)
{
    ULONG flags = krnIPIPortIntrDisable();

    core_HandleCallHookIPI();
    krnIPIPortIntrRestore(flags);
}

void core_SendIPI(ULONG msg, uint32_t mask)
{
    int cpu;

    for (cpu = 0; cpu < KRN_IPICALL_MAXCPUS; cpu++)
        if (mask & (1U << cpu))
            krnIPIPost(cpu, msg);
}

void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg)
{
    krnIPICallCancel(hookEntry, matchArg);
}

int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async,
                   int nargs, IPTR *args, APTR _KB)
{
    int srcCpu = GetCPUNumber();
    ULONG mask;

    (void)_KB;

    if (!hook || !cpu_mask)
        return 0;
    if (nargs > IPI_CALL_HOOK_MAX_ARGS)
        return 0;

    /*
     * TASKAFFINITY_ANY / TASKAFFINITY_ALL_BUT_SELF are sentinels, not
     * buffer pointers - resolve them to bitmaps before dereference.
     */
    if ((IPTR)cpu_mask == TASKAFFINITY_ANY)
        mask = __arm_arosintern.ARMI_AffinityMask;      /* the online cores */
    else if ((IPTR)cpu_mask == TASKAFFINITY_ALL_BUT_SELF)
        mask = __arm_arosintern.ARMI_AffinityMask & ~(1U << srcCpu);
    else
        mask = *(uint32_t *)cpu_mask;

    krnIPICall(hook, &mask, 1, async ? TRUE : FALSE, nargs, args);

    return 1;
}

void handle_ipi(uint32_t ipi, uint32_t ipi_data)
{
    int cpu = GetCPUNumber();
    uint32_t ipi_src = (ipi >> 28) & 0xF;
    uint32_t ipi_msg = ipi & ~(0xF << 28);

    /* What was posted for this core. Taking it here means that a request
     * posted from now on interrupts again. */
    ipi_msg |= krnIPIAck(~0UL);

    D(bug("[Kernel:IPI] %s: Core #%02d IPI Msg %08x:%08x from Core #%02d\n",
        __PRETTY_FUNCTION__, cpu, ipi_msg,  ipi_data, ipi_src));

    /* Bit flags: the mailbox SET register OR-coalesces, so one delivery
     * can carry several messages. Test each bit. */
    if (ipi_msg & IPI_CAUSE)
        D(bug("[Kernel:IPI] IPI_CAUSE:\n"));
    if (ipi_msg & IPI_DISPATCH)
        D(bug("[Kernel:IPI] IPI_DISPATCH:\n"));
    if (ipi_msg & IPI_SWITCH)
        D(bug("[Kernel:IPI] IPI_SWITCH:\n"));
    if (ipi_msg & IPI_SCHEDULE)
    {
        D(bug("[Kernel:IPI] IPI_SCHEDULE:\n"));
        /* Flag for reschedule, and expire the quantum: core_Schedule()
         * keeps the running task while its quantum is unused, so an
         * equal-priority task would never preempt an idle core. */
        FLAG_SCHEDQUANTUM_SET;
        FLAG_SCHEDSWITCH_SET;
    }
    if (ipi_msg & IPI_CALL_HOOK)
    {
        D(bug("[Kernel:IPI] IPI_CALL_HOOK:\n"));
        core_HandleCallHookIPI();
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
