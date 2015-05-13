/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the priority of a task.
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

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
#endif
    BYTE old;

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
    EXEC_SPINLOCK_LOCK(task_listlock, (task->tc_State == TS_READY) ? SPINLOCK_MODE_WRITE : SPINLOCK_MODE_READ);
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

        /*
            I could check the task priorities here to determine if
            the following is really necessary, but OTOH priority
            changes are rare and the hassle isn't really worth it.

            This should be reconsidered, because of Executive [ldp].
        */
        Reschedule();
    }

    /* All done. */
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(task_listlock);
#endif
    Enable();

    return old;

    AROS_LIBFUNC_EXIT
} /* SetTaskPri */
