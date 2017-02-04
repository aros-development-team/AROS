/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/interrupts.h>
#include <exec/rawfmt.h>

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

extern void IdleTask(struct ExecBase *);
AROS_INTP(Exec_X86ShutdownHandler);
AROS_INTP(Exec_X86WarmResetHandler);
AROS_INTP(Exec_X86ColdResetHandler);

int Exec_X86Init(struct ExecBase *SysBase)
{
    struct IntExecBase *sysBase = (struct IntExecBase *)SysBase;
    struct Task *CPUIdleTask;
#if defined(__AROSEXEC_SMP__)
    int cpuNo = KrnGetCPUNumber();
    IPTR idleNameArg[] = 
    {
        cpuNo
    };
#endif
    char *taskName;

    D(bug("[Exec:X86] %s()\n", __PRETTY_FUNCTION__));

    /* Install The default Power Management handlers */
    sysBase->ColdResetHandler.is_Node.ln_Pri = -64;
    sysBase->ColdResetHandler.is_Node.ln_Name = "Keyboard Controller Reset";
    sysBase->ColdResetHandler.is_Code = (VOID_FUNC)Exec_X86ColdResetHandler;
    sysBase->ColdResetHandler.is_Data = &sysBase->ColdResetHandler;
    AddResetCallback(&sysBase->ColdResetHandler);

    sysBase->WarmResetHandler.is_Node.ln_Pri = -64;
    sysBase->WarmResetHandler.is_Node.ln_Name = "System Reset";
    sysBase->WarmResetHandler.is_Code = (VOID_FUNC)Exec_X86WarmResetHandler;
    sysBase->WarmResetHandler.is_Data = &sysBase->WarmResetHandler;
    AddResetCallback(&sysBase->WarmResetHandler);

    sysBase->ShutdownHandler.is_Node.ln_Pri = -128;
    sysBase->ShutdownHandler.is_Node.ln_Name = "System Shutdown";
    sysBase->ShutdownHandler.is_Code = (VOID_FUNC)Exec_X86ShutdownHandler;
    sysBase->ShutdownHandler.is_Data = &sysBase->ShutdownHandler;
    AddResetCallback(&sysBase->ShutdownHandler);

    D(bug("[Exec:X86] %s: Default Handlers Registered\n", __PRETTY_FUNCTION__));

#if defined(__AROSEXEC_SMP__)
    taskName = AllocVec(15, MEMF_CLEAR);
    RawDoFmt("CPU #%03u Idle", (RAWARG)idleNameArg, RAWFMTFUNC_STRING, taskName);
#else
    taskName = "CPU Idle";
#endif
    CPUIdleTask = NewCreateTask(TASKTAG_NAME   , taskName,
#if defined(__AROSEXEC_SMP__)
                                TASKTAG_AFFINITY   , KrnGetCPUMask(cpuNo),
#endif
                                TASKTAG_PRI        , -127,
                                TASKTAG_PC         , IdleTask,
                                TASKTAG_ARG1       , SysBase,
                                TAG_DONE);

    if (CPUIdleTask)
    {
        D(
            bug("[Exec:X86] %s: '%s' Task created @ 0x%p\n", __func__, CPUIdleTask->tc_Node.ln_Name, CPUIdleTask);
#if defined(__AROSEXEC_SMP__)
            bug("[Exec:X86] %s:      CPU Affinity : %08x\n", __func__, IntETask(CPUIdleTask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
#endif
        )
    }

    return TRUE;
}

ADD2INITLIB(Exec_X86Init, 0)
