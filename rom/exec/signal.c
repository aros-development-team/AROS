/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Send some signal to a given task
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "exec_intern.h"

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

#if defined(__AROSEXEC_SMP__)
    spinlock_t *task_listlock = NULL;
#endif

    /* Protect the task lists against other tasks that may use Signal(). */
#if defined(__AROSEXEC_SMP__)
    switch (task->tc_State)
    {
        case TS_RUN:
            task_listlock =&PrivExecBase(SysBase)->TaskRunningSpinLock;
            break;
        case TS_WAIT:
            task_listlock = &PrivExecBase(SysBase)->TaskWaitSpinLock;
            break;
        default:
            task_listlock = &PrivExecBase(SysBase)->TaskReadySpinLock;
            break;
    }
    EXEC_SPINLOCK_LOCK(task_listlock, SPINLOCK_MODE_WRITE);
#endif
    Disable();

    /* Set the signals in the task structure. */
    task->tc_SigRecvd |= signalSet;

    /* Do those bits raise exceptions? */
    if (task->tc_SigExcept & task->tc_SigRecvd)
    {
        /* Yes. Set the exception flag. */
        task->tc_Flags |= TF_EXCEPT;

        /* task is running (Signal() called from within interrupt)? Raise the exception or defer it for later. */
        if (task->tc_State == TS_RUN)
        {
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_UNLOCK(task_listlock);
#endif
            /* Order a reschedule */
            Reschedule();

            /* All done. */
            Enable();

            return;
        }
    }

    /*
        Is the task receiving the signals waiting on them
        (or on a exception) ?
    */
    if ((task->tc_State == TS_WAIT) &&
       (task->tc_SigRecvd&(task->tc_SigWait | task->tc_SigExcept)))
    {
        /* Yes. Move him to the ready list. */
        task->tc_State = TS_READY;
        Remove(&task->tc_Node);
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(task_listlock);
        Enable();
        task_listlock = EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskReadySpinLock, SPINLOCK_MODE_WRITE);
        Disable();
#endif
        Enqueue(&SysBase->TaskReady, &task->tc_Node);

        /* Has it a higher priority as the current one? */
        if (task->tc_Node.ln_Pri > GET_THIS_TASK->tc_Node.ln_Pri)
        {
            /*
                Yes. A taskswitch is necessary. Prepare one if possible.
                (If the current task is not running it is already moved)
            */
            if (GET_THIS_TASK->tc_State == TS_RUN)
            {
#if defined(__AROSEXEC_SMP__)
                EXEC_SPINLOCK_UNLOCK(task_listlock);
                task_listlock = NULL;
#endif
                Reschedule();
            }
        }
    }

#if defined(__AROSEXEC_SMP__)
    if (task_listlock)
    {
        EXEC_SPINLOCK_UNLOCK(task_listlock);
    }
#endif
    Enable();

    AROS_LIBFUNC_EXIT
}

