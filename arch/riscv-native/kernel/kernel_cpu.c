/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#include <aros/types/timespec_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <aros/riscv/cpucontext.h>
#include <strings.h>

#include <aros/types/spinlock_s.h>

#include "kernel_base.h"

#include <proto/kernel.h>

#include "etask.h"

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_cpu.h"
#include <kernel_objects.h>
#include "kernel_syscall.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"

#include "tls.h"

#define D(x)
#define DSCHED(x)
#define DREGS(x)

#if defined(__AROSEXEC_SMP__)
extern struct Task *cpu_InitBootStrap(struct ExecBase *);
extern void cpu_BootStrap(struct Task *, struct ExecBase *);
#endif

void cpu_Init(struct TagItem *msg);

spinlock_t startup_lock;

void cpu_Register()
{
    uint32_t tmp;
#if defined(__AROSEXEC_SMP__)
    tls_t *__tls;
    struct ExecBase *SysBase;
#endif
    struct KernelBase *KernelBase;
    cpuid_t cpunum = GetCPUNumber();

    cpu_Init(NULL);

    KernelBase = (struct KernelBase *)TLS_GET(KernelBase);

    bug("[Kernel:%02d] Operational\n", cpunum);

#if defined(__AROSEXEC_SMP__)
cpu_registerfatal:
#endif
    bug("[Kernel:%02d] Waiting for interrupts\n", cpunum);

    KrnSpinUnLock(&startup_lock);

#if !defined(__AROSEXEC_SMP__)
    do {
#endif
    asm volatile("wfi");
#if !defined(__AROSEXEC_SMP__)
    } while (1);
#else

    /* switch to user mode, and load the bs task stack */
    bug("[Kernel:%02d] Dropping into USER mode ... \n", cpunum);

    /* We now start up the interrupts */
    Permit();
    Enable();
#endif
}

void cpu_Delay(int usecs)
{
    unsigned int delay;
    for (delay = 0; delay < usecs; delay++) asm volatile ("\tnop\n");
}

void cpu_Probe()
{
}

void cpu_Init(struct TagItem *msg)
{
    register unsigned int fpuflags;
    cpuid_t cpunum = GetCPUNumber();

#if (0)
    core_SetupMMU(msg);
#endif
    /* Enable Vector Floating Point Calculations */
#if (0)
#endif
}

void cpu_Switch(regs_t *regs)
{
    struct Task *task;
    UQUAD timeCur;
    struct timespec timeSpec;
    DSCHED(
        cpuid_t cpunum = GetCPUNumber();
        bug("[Kernel:%02d] cpu_Switch()\n", cpunum);
    )

    task = GET_THIS_TASK;

    /* Cache running task's context */
    STORE_TASKSTATE(task, regs)

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;
#if defined(__AROSEXEC_SMP__)
    cpuid_t cpunum = GetCPUNumber();
    DSCHED(
        bug("[Kernel:%02d] cpu_Dispatch()\n", cpunum);
    )
#else
    DSCHED(
        cpuid_t cpunum = GetCPUNumber();
        bug("[Kernel:%02d] cpu_Dispatch()\n", cpunum);
    )
#endif

    while (!(task = core_Dispatch()))
    {
        DSCHED(bug("[Kernel:%02d] cpu_Dispatch: Nothing to run - idling\n", cpunum));
        asm volatile("wfi");
    }

    DSCHED(bug("[Kernel:%02d] cpu_Dispatch: 0x%p [R  ] '%s'\n", cpunum, task, task->tc_Node.ln_Name));

    /* Restore the task's state */
    RESTORE_TASKSTATE(task, regs)

    /* Handle tasks's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();

#if defined(__AROSEXEC_SMP__)
    IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber = cpunum;
#endif

    if (task->tc_Flags & TF_LAUNCH)
    {
        AROS_UFC1(void, task->tc_Launch,
                  AROS_UFCA(struct ExecBase *, SysBase, A6));
    }
    /* Leave interrupt and jump to the new task */
}
