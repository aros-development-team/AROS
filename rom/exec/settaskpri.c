/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Change the priority of a task.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#include "exec_locks.h"
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

    struct Task *thisTask = GET_THIS_TASK;
#if defined(__AROSEXEC_SMP__)
    spinlock_t *task_listlock = NULL;
    BOOL doResched = FALSE, doRemote = FALSE;
    int cpunum = KrnGetCPUNumber();
    void *remoteCpuAffinity = NULL;
#endif
    BYTE old;

    D(bug("[Exec] SetTaskPri(0x%p, %d)\n", task, priority);)

    /* Always Disable() when doing something with task lists. */
    Disable();
#if defined(__AROSEXEC_SMP__)
    /*
     * Take the task lock before picking the list lock: tc_State tells us
     * which list the task is on, and only the task lock keeps it still.
     */
    EXEC_SPINLOCK_LOCK(&task->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE);
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
    if (task->tc_State == TS_READY)
        EXEC_LOCK_WRITE(task_listlock);
    else
        EXEC_LOCK_READ(task_listlock);
#endif

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

#if defined(__AROSEXEC_SMP__)
        if (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber == cpunum)
            doResched = (task->tc_Node.ln_Pri > thisTask->tc_Node.ln_Pri);
        else if (task->tc_State == TS_RUN)
        {
            /* Read the affinity while still locked - the ETask can be
             * freed the moment we let go. */
            remoteCpuAffinity = IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity;
            doRemote = TRUE;
        }
#else
        if ( task->tc_Node.ln_Pri > thisTask->tc_Node.ln_Pri)
        {
            D(bug("[Exec] SetTaskPri: Task needs reschedule...\n");)
            Reschedule();
        }
#endif
    }

    /* All done. */
#if defined(__AROSEXEC_SMP__)
    /* Drop both locks before kicking any scheduler path. */
    EXEC_UNLOCK(task_listlock);
    EXEC_SPINLOCK_UNLOCK(&task->tc_SpinLock);

    if (doResched)
    {
        D(bug("[Exec] SetTaskPri: Task needs reschedule...\n");)
        Reschedule();
    }
    else if (doRemote)
    {
        D(bug("[Exec] SetTaskPri: changing priority of task running on another cpu (%03u)\n", remoteCpuNumber);)
        KrnScheduleCPU(remoteCpuAffinity);
    }
#endif
    Enable();

    return old;

    AROS_LIBFUNC_EXIT
} /* SetTaskPri */
