/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the priority of a task.
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

/*****************************************************************************

    NAME */

        AROS_LH2(BYTE, SetTaskPri,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task,      A1),
        AROS_LHA(LONG,          priority,  D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 50, Exec)

/*  FUNCTION
        Change the priority of a given task. As a general rule the higher
        the priority the more CPU time a task gets. Useful values are within
        -127 to 5.

    INPUTS
        task     - Pointer to task structure.
        priority - New priority of the task.

    RESULT
        Old task priority.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    spinlock_t *task_listlock = NULL;
    int cpunum = KrnGetCPUNumber();
#endif
    BYTE old;

    D(bug("[Exec] SetTaskPri(0x%p, %d)\n", task, priority);)

    /* Always Disable() when doing something with task lists. */
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
    EXECTASK_SPINLOCK_LOCK(task_listlock, (task->tc_State == TS_READY) ? SPINLOCK_MODE_WRITE : SPINLOCK_MODE_READ);
#endif
    Disable();

    /* Get returncode */
    old = task->tc_Node.ln_Pri;

    /* Set new value. */
    task->tc_Node.ln_Pri = priority;

    /* Check if the task is willing to run. */
    if (task->tc_State != TS_WAIT)
    {
        /* If it is in the ready list remove and reinsert it. */
        if (task->tc_State == TS_READY)
        {
            Remove(&task->tc_Node);
            Enqueue(&SysBase->TaskReady, &task->tc_Node);
        }

        if (
#if defined(__AROSEXEC_SMP__)
             (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber == cpunum) && 
#endif
            ((task->tc_State == TS_RUN) || ( task->tc_Node.ln_Pri > GET_THIS_TASK->tc_Node.ln_Pri))
        )
        {
#if defined(__AROSEXEC_SMP__)
            EXECTASK_SPINLOCK_UNLOCK(task_listlock);
            task_listlock = NULL;
#endif
            Reschedule();
        }
#if defined(__AROSEXEC_SMP__)
        else
        {
            bug("[Exec] SetTaskPri:\n");
        }
#endif
    }

    /* All done. */
#if defined(__AROSEXEC_SMP__)
    if (task_listlock)
    {
        EXECTASK_SPINLOCK_UNLOCK(task_listlock);
    }
#endif
    Enable();

    return old;

    AROS_LIBFUNC_EXIT
} /* SetTaskPri */
