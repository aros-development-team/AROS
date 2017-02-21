/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Send some signal to a given task
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

#if defined(__AROSEXEC_SMP__)
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#endif

/*****************************************************************************

    NAME */

        AROS_LH2(void, Signal,

/*  SYNOPSIS */
        AROS_LHA(struct Task *,     task,      A1),
        AROS_LHA(ULONG,             signalSet, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 54, Exec)

/*  FUNCTION
        Send some signals to a given task. If the task is currently waiting
        on these signals, has a higher priority as the current one and if
        taskswitches are allowed the new task begins to run immediately.

    INPUTS
        task      - Pointer to task structure.
        signalSet - The set of signals to send to the task.

    RESULT

    NOTES
        This function may be used from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
        AllocSignal(), FreeSignal(), Wait(), SetSignal(), SetExcept()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *thisTask = GET_THIS_TASK;
#if defined(__AROSEXEC_SMP__)
    int cpunum = KrnGetCPUNumber();
#endif

    D(
        bug("[Exec] Signal(0x%p, %08lX)\n", task, signalSet);
        bug("[Exec] Signal: signaling '%s' (state %08x)\n", task->tc_Node.ln_Name, task->tc_State);
        bug("[Exec] Signal: from '%s'\n", thisTask->tc_Node.ln_Name);
    )

    Disable();
    /* Set the signals in the task structure. */
#if defined(__AROSEXEC_SMP__)
    __AROS_ATOMIC_OR_L(task->tc_SigRecvd, signalSet);
#else
    task->tc_SigRecvd |= signalSet;
#endif

    /* Do those bits raise exceptions? */
    if (task->tc_SigRecvd & task->tc_SigExcept)
    {
        /* Yes. Set the exception flag. */
#if defined(__AROSEXEC_SMP__)
        __AROS_ATOMIC_OR_B(task->tc_Flags, TF_EXCEPT);
#else
        task->tc_Flags |= TF_EXCEPT;
#endif

        D(bug("[Exec] Signal: TF_EXCEPT set\n");)
    }

    /* 
            if the target task is running (called from within interrupt handler),
            raise the exception or defer it for later.
        */
    if ((task->tc_SigRecvd & task->tc_SigExcept) &&
        (task->tc_State == TS_RUN))
    {
#if defined(__AROSEXEC_SMP__)
        if (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber == cpunum)
        {
#endif
        D(bug("[Exec] Signal: signaling running task\n");)
        /* Order a reschedule */
        Reschedule();
#if defined(__AROSEXEC_SMP__)
        }
        else
        {
            D(bug("[Exec] Signal: signaling task on another cpu (%03u)\n", IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber);)
            KrnScheduleCPU(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
        }
#endif
        Enable();

        /* All done. */
        return;
    }

    /*
        Is the task receiving the signals waiting on them
        (or on a exception) ?
    */
    if ((task->tc_State == TS_WAIT) &&
       (task->tc_SigRecvd & (task->tc_SigWait | task->tc_SigExcept)))
    {
        D(bug("[Exec] Signal: signaling waiting task\n");)

        /* Yes. Move it to the ready list. */
#if defined(__AROSEXEC_SMP__)
        krnSysCallReschedTask(task, TS_READY);
#else
        Remove(&task->tc_Node);
        task->tc_State = TS_READY;
        Enqueue(&SysBase->TaskReady, &task->tc_Node);
#endif
        /* Has it a higher priority as the current one? */
        if (
#if defined(__AROSEXEC_SMP__)
            (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity & KrnGetCPUMask(cpunum)) &&
#endif
            (task->tc_Node.ln_Pri > thisTask->tc_Node.ln_Pri))
        {
            /*
                Yes. A taskswitch is necessary. Prepare one if possible.
                (If the current task is not running it is already moved)
            */
            if (thisTask->tc_State == TS_RUN)
            {
                Reschedule();
            }
        }
#if defined(__AROSEXEC_SMP__)
        else if (!(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity & KrnGetCPUMask(cpunum)))
        {
            krnSysCallReschedTask(task, TS_READY);
            KrnScheduleCPU(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
        }
#endif
    }

    Enable();

    D(bug("[Exec] Signal: 0x%p finished signal processing\n", task);)

    AROS_LIBFUNC_EXIT
}

