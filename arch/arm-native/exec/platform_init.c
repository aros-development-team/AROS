/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 1

#include <aros/debug.h>
#include <aros/cpu.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <strings.h>
#include <stdio.h>

#include "kernel_cpu.h"
#include "kernel_ipi.h"

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

/* Linked from kernel.resource,
 * need to retrieve in a cleaner fashion .. */
extern IPTR stack[];

extern void IdleTask(struct ExecBase *);

int Exec_ARMCPUInit(struct ExecBase *SysBase)
{
    struct Task *BootTask, *CPUIdleTask;
#if defined(__AROSEXEC_SMP__)
    int cpu, cpunum = KrnGetCPUCount();
    void *cpuMask = NULL;
#endif
    char *taskName;

    D(bug("[Exec] %s()\n", __PRETTY_FUNCTION__));

    BootTask = GET_THIS_TASK;

    D(bug("[Exec] %s: launched from %s @ 0x%p\n", __PRETTY_FUNCTION__, BootTask->tc_Node.ln_Name, BootTask));

#if defined(__AROSEXEC_SMP__)
    if (cpunum == 0)
    {
#endif
        /* for our sanity we will tell exec about the correct stack for the boot task */
        BootTask->tc_SPLower = stack;
        BootTask->tc_SPUpper = stack + AROS_STACKSIZE;
#if defined(__AROSEXEC_SMP__)
    }

    for (cpu = 0; cpu < cpunum; cpu ++)
    {
        taskName = AllocVec(15, MEMF_CLEAR);
        sprintf( taskName, "CPU #%02d Idle", cpu);
        cpuMask = KrnAllocCPUMask();
        if (cpuMask)
            KrnGetCPUMask(cpu, cpuMask);
#else
    taskName = "System Idle";
#endif
        CPUIdleTask = NewCreateTask(TASKTAG_NAME   , taskName,
#if defined(__AROSEXEC_SMP__)
                                TASKTAG_AFFINITY   , cpuMask,
#endif
                                TASKTAG_PRI        , -127,
                                TASKTAG_PC         , IdleTask,
                                TASKTAG_ARG1       , SysBase,
                                TAG_DONE);

        if (CPUIdleTask)
        {
            D(
                bug("[Exec] %s: %s Task created @ 0x%p\n", __PRETTY_FUNCTION__, CPUIdleTask->tc_Node.ln_Name, CPUIdleTask);
#if defined(__AROSEXEC_SMP__)
                bug("[Exec] %s: CPU Affinity : %08x\n", __PRETTY_FUNCTION__, GetIntETask(CPUIdleTask)->iet_CpuAffinity);
#endif
            )
#if defined(__AROSEXEC_SMP__)
            /*
             * Tell the kernel which task idles this core, so
             * KrnGetSystemAttr(KATTR_CPULoad + cpu) can derive the core's
             * load from it. Only exec knows the mapping.
             */
            if (cpu < ARM_MAXCPUS)
                arm_IdleTask[cpu] = CPUIdleTask;
#endif
        }
#if defined(__AROSEXEC_SMP__)
    }
#endif

    return TRUE;
}

#if defined(__AROSEXEC_SMP__)
/*
 * The hook a caller can pass to KrnSpinLock() for a lock it may have to
 * wait long for (as on all-pc): the scheduler is told that the task only
 * spins, and the task gives up the CPU until it is scheduled again.
 */
struct Hook Exec_TaskSpinLockFailHook;

AROS_UFH3(void, Exec_TaskSpinLockFailFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(spinlock_t *, spinLock, A1),
    AROS_UFHA(void *, unused, A2))
{
    AROS_USERFUNC_INIT

    struct Task *spinTask = GET_THIS_TASK;
    struct IntETask *thisET;

    if (spinTask)
    {
        thisET = GetIntETask(spinTask);
        if (thisET)
        {
            /* tell the scheduler that the task is waiting on a spinlock */
            spinTask->tc_State = TS_SPIN;

            thisET->iet_SpinLock = spinLock;
        }
    }

    /* schedule us away for now .. */
    Reschedule();

    AROS_USERFUNC_EXIT
}

int Exec_ARMCPUSMPInit(struct ExecBase *SysBase)
{
    /* set up the task spinning hook */
    Exec_TaskSpinLockFailHook.h_Entry = (HOOKFUNC)Exec_TaskSpinLockFailFunc;

    /*
     * Turn on CPU affinity routing in arch-neutral exec. Without this
     * flag Signal() never sends a signal to the CPU that owns the task
     * (core_DoCallIPI), and a task on another CPU is not woken. The
     * cpumask helpers (alloc/get/in/clear/free) and core_DoCallIPI must
     * already work when it is set.
     */
    PrivExecBase(SysBase)->IntFlags |= EXECF_CPUAffinity;

    /*
     * Pin the primordial boot/init task to the boot CPU. It was created
     * (exec_init.c) before EXECF_CPUAffinity was set, so InitETask left it
     * with a NULL affinity == "run anywhere". That lets a secondary core
     * dispatch it mid-init, but the single-threaded coldstart sequence is
     * not migration-safe (it holds boot-CPU/boot-stack-local state, e.g.
     * stack-resident hooks), so running it elsewhere crashes. Keep it on
     * the boot CPU (always logical 0); other tasks distribute normally.
     */
    {
        struct Task *bootTask = GET_THIS_TASK;
        struct IntETask *iet = bootTask ? (struct IntETask *)GetETask(bootTask) : NULL;

        if (iet && !iet->iet_CpuAffinity)
        {
            void *aff = KrnAllocCPUMask();
            if (aff)
            {
                KrnGetCPUMask(0, aff);
                iet->iet_CpuAffinity = aff;
                iet->iet_CpuNumber = 0;
            }
        }
    }

    return TRUE;
}

ADD2INITLIB(Exec_ARMCPUSMPInit, -127)
#endif

ADD2INITLIB(Exec_ARMCPUInit, 0)
