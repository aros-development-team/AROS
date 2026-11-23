/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    AArch64 exec platform initialization.
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
#include <exec/rawfmt.h>
#include <asm/io.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <strings.h>

#include "kernel_cpu.h"
#include "kernel_ipi.h"

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

/* Linked from kernel.resource */
extern IPTR stack[];
extern IPTR stack_super[];

extern void IdleTask(struct ExecBase *);

struct Task *Exec_ARMCreateIdleTask(struct ExecBase *SysBase)
{
    struct Task *CPUIdleTask;
    char *taskName;
#if defined(__AROSEXEC_SMP__)
    cpuid_t cpuNo = KrnGetCPUNumber();
    IPTR idleNameArg[] =
    {
        cpuNo
    };
    struct MemList *ml;
    void *cpuMask;

    /* Not optional: affinity 0 would mean the boot CPU, not this one */
    if ((cpuMask = KrnAllocCPUMask()) == NULL)
    {
        bug("[Exec:%02u] FATAL : Failed to allocate the idle task's CPU mask\n", cpuNo);
        return NULL;
    }

    if ((ml = AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Exec:%02u] FATAL : Failed to allocate memory for idle task name info\n", cpuNo);
        KrnFreeCPUMask(cpuMask);
        return NULL;
    }

    ml->ml_NumEntries      = 1;

    ml->ml_ME[0].me_Length = 15;
    if ((ml->ml_ME[0].me_Addr = AllocMem(15, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Exec:%02u] FATAL : Failed to allocate memory for task idle name\n", cpuNo);
        FreeMem(ml, sizeof(struct MemList));
        KrnFreeCPUMask(cpuMask);
        return NULL;
    }
    taskName = ml->ml_ME[0].me_Addr;
    RawDoFmt("CPU #%02u Idle", (RAWARG)idleNameArg, RAWFMTFUNC_STRING, taskName);

    KrnGetCPUMask(cpuNo, cpuMask);
#else
    taskName = "System Idle";
#endif
    CPUIdleTask = NewCreateTask(TASKTAG_NAME   , taskName,
#if defined(__AROSEXEC_SMP__)
                                TASKTAG_AFFINITY   , cpuMask,
#endif
                                TASKTAG_PRI        , -128,
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
        AddTail(&CPUIdleTask->tc_MemEntry, &ml->ml_Node);
        /* KATTR_CPULoad derives a core's load from its idle task,
         * and only exec knows the mapping. Other cores read it. */
        if (cpuNo < AARCH64_MAXCPUS)
            __atomic_store_n(&aarch64_IdleTask[cpuNo], CPUIdleTask, __ATOMIC_RELEASE);
#endif
    }
#if defined(__AROSEXEC_SMP__)
    else
    {
        FreeMem(ml->ml_ME[0].me_Addr, 15);
        FreeMem(ml, sizeof(struct MemList));
    }
#endif

    return CPUIdleTask;
}

int Exec_ARMCPUInit(struct ExecBase *SysBase)
{
    struct Task *BootTask;

    D(bug("[Exec] %s()\n", __PRETTY_FUNCTION__));

    BootTask = GET_THIS_TASK;

    D(bug("[Exec] %s: launched from %s @ 0x%p\n", __PRETTY_FUNCTION__, BootTask->tc_Node.ln_Name, BootTask));

    /* The boot task runs on the kernel's boot stack. (Under SMP this was
     * only done when KrnGetCPUCount() returned 0, which it never does.) */
    BootTask->tc_SPLower = stack;
    BootTask->tc_SPUpper = stack + AROS_STACKSIZE;

    /* The other cores create theirs as they start (cpu_Register) */
    return (Exec_ARMCreateIdleTask(SysBase) != NULL);
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

/* Pin a task that predates EXECF_CPUAffinity to the boot CPU */
static BOOL Exec_ARMAssignBootMask(struct Task *task)
{
    struct IntETask *iet;
    void *aff;

    if (!task || !(task->tc_Flags & TF_ETASK) || !task->tc_UnionETask.tc_ETask)
        return TRUE;

    iet = IntETask(task->tc_UnionETask.tc_ETask);
    if (iet->iet_CpuAffinity)
        return TRUE;

    if (!(aff = KrnAllocCPUMask()))
        return FALSE;
    KrnGetCPUMask(0, aff);
    iet->iet_CpuAffinity = aff;
    iet->iet_CpuNumber = 0;

    return TRUE;
}

int Exec_ARMCPUSMPInit(struct ExecBase *SysBase)
{
    struct Task *task;
    BOOL ok;

    /* set up the task spinning hook */
    Exec_TaskSpinLockFailHook.h_Entry = (HOOKFUNC)Exec_TaskSpinLockFailFunc;

    /*
     * Every task that predates EXECF_CPUAffinity gets a mask first (the
     * coldstart sequence is not migration-safe, so the boot CPU's), as
     * all-pc does: a NULL mask is invalid and would leave a task no CPU
     * dispatches. Only then is the gate for signal.c's cross-CPU paths
     * opened.
     */
    Disable();
    ok = Exec_ARMAssignBootMask(GET_THIS_TASK);
    ForeachNode(&SysBase->TaskReady, task)
        ok = ok && Exec_ARMAssignBootMask(task);
    ForeachNode(&SysBase->TaskWait, task)
        ok = ok && Exec_ARMAssignBootMask(task);
    if (ok)
        PrivExecBase(SysBase)->IntFlags |= EXECF_CPUAffinity;
    Enable();

    if (!ok)
        bug("[Exec] %s: no memory for task affinity masks\n", __PRETTY_FUNCTION__);

    return TRUE;
}

ADD2INITLIB(Exec_ARMCPUSMPInit, -127)
#endif

ADD2INITLIB(Exec_ARMCPUInit, 0)
