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
#include <proto/kernel.h>
#include "etask.h"
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

    struct Task *ThisTask = GET_THIS_TASK;
#if defined(__AROSEXEC_SMP__)
    spinlock_t *task_listlock = NULL;
    int cpunum = KrnGetCPUNumber();
#endif

    D(
        bug("[Exec] Signal(0x%p, %08lX)\n", task, signalSet);
        bug("[Exec] Signal: signaling '%s' (state %08x)\n", task->tc_Node.ln_Name, task->tc_State);
        bug("[Exec] Signal: from '%s'\n", ThisTask->tc_Node.ln_Name);
    )

#if defined(__AROSEXEC_SMP__)
    EXECTASK_SPINLOCK_LOCK(&IntETask(task->tc_UnionETask.tc_ETask)->iet_TaskLock, SPINLOCK_MODE_WRITE);
#endif
    Disable();
    /* Set the signals in the task structure. */
    task->tc_SigRecvd |= signalSet;
#if defined(__AROSEXEC_SMP__)
    EXECTASK_SPINLOCK_UNLOCK(&IntETask(task->tc_UnionETask.tc_ETask)->iet_TaskLock);
    Enable();
#endif

    /* Do those bits raise exceptions? */
    if (task->tc_SigRecvd & task->tc_SigExcept)
    {
        /* Yes. Set the exception flag. */
        task->tc_Flags |= TF_EXCEPT;

        D(bug("[Exec] Signal: TF_EXCEPT set\n");)

        /* 
                if the target task is running (called from within interrupt handler),
                raise the exception or defer it for later.
            */
        if (task->tc_State == TS_RUN)
        {
            D(bug("[Exec] Signal: signaling running task\n");)
#if defined(__AROSEXEC_SMP__)
            if (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber == cpunum)
            {
#endif
            /* Order a reschedule */
            Reschedule();
#if defined(__AROSEXEC_SMP__)
            }
            else
            {
                D(bug("[Exec] Signal:\n");)
            }
#else
            Enable();
#endif

            /* All done. */
            return;
        }
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
        task_listlock = &PrivExecBase(SysBase)->TaskWaitSpinLock;
        EXECTASK_SPINLOCK_LOCK(task_listlock, SPINLOCK_MODE_WRITE);
        Forbid();
#endif
        Remove(&task->tc_Node);
        task->tc_State = TS_READY;
#if defined(__AROSEXEC_SMP__)
        EXECTASK_SPINLOCK_UNLOCK(task_listlock);
        Permit();
        task_listlock = EXECTASK_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskReadySpinLock, SPINLOCK_MODE_WRITE);
        Forbid();
#endif
        Enqueue(&SysBase->TaskReady, &task->tc_Node);
#if defined(__AROSEXEC_SMP__)
        EXECTASK_SPINLOCK_UNLOCK(task_listlock);
        Permit();
        task_listlock = NULL;
#endif
        /* Has it a higher priority as the current one? */
        if (
#if defined(__AROSEXEC_SMP__)
            (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity & KrnGetCPUMask(cpunum)) &&
#endif
            (task->tc_Node.ln_Pri > ThisTask->tc_Node.ln_Pri))
        {
            /*
                Yes. A taskswitch is necessary. Prepare one if possible.
                (If the current task is not running it is already moved)
            */
            if (ThisTask->tc_State == TS_RUN)
            {
                Reschedule();
            }
        }
    }

#if !defined(__AROSEXEC_SMP__)
    Enable();
#endif

    D(bug("[Exec] Signal: 0x%p finished signal processing\n", task);)

    AROS_LIBFUNC_EXIT
}

