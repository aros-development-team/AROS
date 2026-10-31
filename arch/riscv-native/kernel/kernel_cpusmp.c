/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Secondary hart bring-up for native RISC-V SMP kernels.

    The boot CPU starts the other harts one at a time, each through the
    platform's RVI_StartCPU hook, and waits for it to report in under
    startup_lock, as the ARM ports do. A hart enters cpu_Register() on
    the stack its TLS names, with tp, gp, stvec and satp already set up
    by the platform entry code. There it builds a bootstrap task and joins
    the scheduler; the bootstrap task creates the hart's idle task,
    releases startup_lock and exits, as all-pc's does.
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/tasks.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include "etask.h"
#define __AROS_KERNEL__
#include "exec_intern.h"
#include "kernel_riscv.h"
#include "kernel_spinlock.h"
#include "kernel_ipi.h"

#undef D
#define D(x)

/* The stack a secondary hart runs cpu_Register() on, before its task,
 * and then takes its traps on (traps.S) - as large as all-pc's */
#define SECONDARY_STACKSIZE     65536

extern struct Task *cpu_InitBootStrap(struct ExecBase *SysBase, uint32_t cpunum);
extern void cpu_DisposeBootStrap(struct Task *bstask, struct ExecBase *SysBase);
extern void cpu_BootStrap(struct Task *bstask, struct ExecBase *SysBase);
/* exec_idle.c - kernel and exec share one core image */
extern struct Task *Exec_RVCreateIdleTask(struct ExecBase *SysBase);

static spinlock_t startup_lock;

static inline UQUAD krnRVTime(void)
{
    UQUAD v;

    asm volatile("rdtime %0" : "=r"(v));
    return v;
}

/*
 * The bootstrap task's code: it only gets scheduling going on this hart.
 * It returns into exec's task exit code like any other task, and this
 * hart's first dispatch then finds its idle task.
 */
static void cpu_BootStrapEntry(void)
{
    struct ExecBase *SysBase = TLS_GET(SysBase);

    /* Now this hart takes interrupts and so schedules */
    Permit();
    Enable();

    if (!Exec_RVCreateIdleTask(SysBase))
        bug("[Kernel:%02u] FATAL : no idle task\n", krnRVCPUNumber());

    /* Only now may the boot CPU bring up the next hart (as all-pc's
     * ready lock): that one allocates before it has a task. */
    Kernel_53_KrnSpinUnLock(&startup_lock, NULL);
}

/*
 * C entry of a secondary hart (from the platform's secondary entry).
 * Does not return.
 */
void __attribute__((noreturn)) cpu_Register(void)
{
    tls_t *tls = TLS_PTR_GET();
    struct ExecBase *SysBase = tls->SysBase;
    struct KernelBase *KernelBase = (struct KernelBase *)tls->KernelBase;
    uint32_t cpunum = tls->CPUNumber;
    struct Task *bstask;

    bug("[Kernel:%02u] Hart %lu bootstrapping, TLS @ 0x%p\n",
        cpunum, (unsigned long)tls->HartID, tls);

    if (__riscv_arosintern.RVI_InitCore)
        __riscv_arosintern.RVI_InitCore(KernelBase, SysBase);

    /* Built for this hart by the boot CPU (krnRVStartCPUs) */
    if ((bstask = tls->ThisTask) == NULL)
    {
        bug("[Kernel:%02u] FATAL : no bootstrap task - hart parked\n", cpunum);
        Kernel_53_KrnSpinUnLock(&startup_lock, NULL);
        for (;;)
            asm volatile("wfi");
    }
    cpu_BootStrap(bstask, SysBase);

    /* Schedulable from here: exec may give this CPU work */
    krnRVCPUOnline(cpunum);
    if (PrivExecBase(SysBase)->CPUMask)
        KrnGetCPUMask(cpunum, PrivExecBase(SysBase)->CPUMask);

    /* Inter-processor interrupts, then this hart's own tick */
    asm volatile("csrs sie, %0" :: "r"(2UL) : "memory");    /* SIE.SSIE */
    if (__riscv_arosintern.RVI_InitTimerCore)
        __riscv_arosintern.RVI_InitTimerCore();

    bug("[Kernel:%02u] Operational\n", cpunum);

    /*
     * Continue as the bootstrap task, on its own stack, so that it is the
     * context the first trap saves. Its code returns to TaskExitCode, as
     * every task's does (PrepareContext).
     */
    asm volatile(
        "mv     sp, %0  \n"
        "mv     ra, %1  \n"
        "tail   %2      \n"
        : : "r"(bstask->tc_SPUpper), "r"(SysBase->TaskExitCode),
            "i"(cpu_BootStrapEntry)
        : "memory");

    __builtin_unreachable();
}

/*
 * Start every hart the platform can offer. Called by kernel.post
 * (platform_post.c) once exec is fully up. Every task that exists has
 * an affinity mask by then (exec_smp.c), so a new CPU can only pick up
 * work that is allowed on it; without them, stay on one CPU.
 */
void krnRVStartCPUs(struct KernelBase *KernelBase, struct ExecBase *SysBase)
{
    UQUAD timebase = __riscv_arosintern.RVI_TimebaseFreq ?
                     __riscv_arosintern.RVI_TimebaseFreq : 10000000;
    uint32_t cpu = 1;

    if (!__riscv_arosintern.RVI_StartCPU ||
        !(PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity))
        return;

    core_IPIInit();

    while (cpu < RISCV_MAXCPUS)
    {
        tls_t *tls;
        APTR stack;
        UQUAD deadline;
        BOOL released;
        int r;

        tls = AllocMem(sizeof(tls_t), MEMF_PUBLIC | MEMF_CLEAR);
        stack = AllocMem(SECONDARY_STACKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
        if (!tls || !stack)
        {
            if (tls)
                FreeMem(tls, sizeof(tls_t));
            if (stack)
                FreeMem(stack, SECONDARY_STACKSIZE);
            bug("[Kernel] %s: out of memory for CPU #%02u\n", __func__, cpu);
            break;
        }

        tls->SysBase = SysBase;
        tls->KernelBase = (void *)KernelBase;
        tls->CPUNumber = cpu;
        tls->StackTop = (APTR)(((IPTR)stack + SECONDARY_STACKSIZE) & ~(IPTR)15);
        /* Disable()d and Forbid()den until cpu_BootStrapEntry lifts both */
        tls->IDNestCnt = 0;
        tls->TDNestCnt = 0;
        /* exec keeps the quantum per CPU; start with the boot CPU's */
        tls->Quantum = __riscv_boot_tls.Quantum;
        tls->Elapsed = tls->Quantum;
        /* The hart itself must not allocate before it has a task */
        if ((tls->ThisTask = cpu_InitBootStrap(SysBase, cpu)) == NULL)
        {
            FreeMem(stack, SECONDARY_STACKSIZE);
            FreeMem(tls, sizeof(tls_t));
            bug("[Kernel] %s: out of memory for CPU #%02u\n", __func__, cpu);
            break;
        }
        __riscv_tls[cpu] = tls;

        Kernel_52_KrnSpinLock(&startup_lock, NULL, SPINLOCK_MODE_WRITE, NULL);
        asm volatile("fence rw, rw" ::: "memory");

        r = __riscv_arosintern.RVI_StartCPU(cpu, tls);
        if (r != 0)
        {
            Kernel_53_KrnSpinUnLock(&startup_lock, NULL);
            __riscv_tls[cpu] = NULL;
            cpu_DisposeBootStrap(tls->ThisTask, SysBase);
            FreeMem(stack, SECONDARY_STACKSIZE);
            FreeMem(tls, sizeof(tls_t));
            if (r < 0)
                break;          /* no harts left */
            continue;           /* that hart refused - try the next one */
        }

        /* The hart releases the lock once its idle task exists */
        deadline = krnRVTime() + timebase;
        while (!(released = krnRVSpinTryWrite(&startup_lock)))
        {
            if ((QUAD)(krnRVTime() - deadline) > 0)
                break;
            krnRVPause();
        }

        if (!released || !(__riscv_cpu_online[cpu / 32] & (1UL << (cpu % 32))))
        {
            /*
             * It did not report in within a second. It may still be on
             * its way and owns its TLS and stack, so neither can be
             * reclaimed, and starting more harts behind it would race
             * it for the lock: stop here.
             */
            bug("[Kernel] %s: CPU #%02u did not come up - not starting more\n",
                __func__, cpu);
            break;
        }

        Kernel_53_KrnSpinUnLock(&startup_lock, NULL);
        cpu++;
    }

    bug("[Kernel] %u CPU(s) online\n", __riscv_cpu_count);
}

#endif /* __AROSEXEC_SMP__ */
