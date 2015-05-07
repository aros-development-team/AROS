/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
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

#include "exec_intern.h"

/* Linked from kernel.resource,
 * need to retrieve in a cleaner fashion .. */
extern IPTR stack[];

extern void IdleTask(struct ExecBase *);

int Exec_ARMCPUInit(struct ExecBase *SysBase)
{
    struct Task *BootTask, *CPUIdleTask;
    int cpunum = KrnGetCPUNumber();

    D(bug("[Exec] Exec_ARMCPUInit(%02d)\n", cpunum));

    BootTask = GET_THIS_TASK;

    D(bug("[Exec] Exec_ARMCPUInit[%02d]: launched from %s @ 0x%p\n", cpunum, BootTask->tc_Node.ln_Name, BootTask));

    if (cpunum == 0)
    {
        /* for our sanity we will tell exec about the correct stack for the boot task */
        BootTask->tc_SPLower = stack;
        BootTask->tc_SPUpper = stack + AROS_STACKSIZE;
    }

    CPUIdleTask = NewCreateTask(TASKTAG_NAME       , "System Idle",
#if defined(__AROSEXEC_SMP__)
                                TASKTAG_AFFINITY   , KrnGetCPUMask(cpunum),
#endif
                                TASKTAG_PRI        , -127,
                                TASKTAG_PC         , IdleTask,
                                TASKTAG_ARG1       , SysBase,
                                TAG_DONE);

    if (CPUIdleTask)
    {
        D(bug("[Exec] Exec_ARMCPUInit[%02d]: %s Task created @ 0x%p\n", cpunum, CPUIdleTask->tc_Node.ln_Name, CPUIdleTask));
    }

    return TRUE;
}

ADD2INITLIB(Exec_ARMCPUInit, 0)
