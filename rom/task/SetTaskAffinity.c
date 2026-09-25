/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/tasks.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <resources/task.h>

#include "etask.h"

#include "task_intern.h"

/*****************************************************************************

    NAME */
#include <proto/task.h>

        AROS_LH2(BOOL, SetTaskAffinity,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task,     A0),
        AROS_LHA(APTR,          affinity, A1),

/*  LOCATION */
        struct TaskResBase *, TaskResBase, 21, Task)

/*  FUNCTION
        Change the set of CPUs a task may run on.

    INPUTS
        task     - The task to modify, or NULL for the calling task.
        affinity - A KrnAllocCPUMask() mask, or TASKAFFINITY_ANY for
                   every CPU. The mask is copied; the caller keeps it.

    RESULT
        TRUE if the affinity was changed, FALSE otherwise.

    NOTES
        A NULL mask, or one naming no online CPU, is rejected.

        A task running on an excluded CPU moves at that CPU's next
        scheduling point.

        Always fails on non-SMP builds.

    EXAMPLE

    BUGS

    SEE ALSO
        QueryTaskTagList(), KrnAllocCPUMask(), KrnGetCPUMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    struct IntETask *iet;
    APTR cur, fresh = NULL;
    BOOL any = ((IPTR)affinity == TASKAFFINITY_ANY);
    BOOL kick = FALSE;
    int i, count;

    if (!task)
        task = FindTask(NULL);

    if (!task || !(task->tc_Flags & TF_ETASK) || !task->tc_UnionETask.tc_ETask ||
        !affinity)
        return FALSE;

    iet = IntETask(task->tc_UnionETask.tc_ETask);
    count = KrnGetCPUCount();

    if (!any)
    {
        for (i = 0; i < count; i++)
            if (KrnCPUInMask(i, affinity))
                break;

        if (i == count)
        {
            D(bug("[TaskRes] %s: mask names no online CPU\n", __func__);)
            return FALSE;
        }
    }

    /* The scheduler reads a task's mask without its lock, so an allocated
     * mask is rewritten in place and only freed with the task. A task
     * without one gets a fresh mask, allocated before taking the lock. */
    cur = (APTR)iet->iet_CpuAffinity;
    if (!any && (!cur || (IPTR)cur == TASKAFFINITY_ANY))
    {
        if (!(fresh = KrnAllocCPUMask()))
            return FALSE;
        for (i = 0; i < count; i++)
            if (KrnCPUInMask(i, affinity))
                KrnGetCPUMask(i, fresh);
    }

    Disable();
    EXEC_SPINLOCK_LOCK(&task->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE);

    cur = (APTR)iet->iet_CpuAffinity;
    if (cur && (IPTR)cur != TASKAFFINITY_ANY)
    {
        KrnClearCPUMask(cur);
        for (i = 0; i < count; i++)
            if (any || KrnCPUInMask(i, affinity))
                KrnGetCPUMask(i, cur);
    }
    else if (fresh)
    {
        iet->iet_CpuAffinity = cur = fresh;
        fresh = NULL;
    }
    else
        iet->iet_CpuAffinity = cur = (APTR)TASKAFFINITY_ANY;

    if ((IPTR)cur != TASKAFFINITY_ANY && task->tc_State == TS_RUN &&
        !KrnCPUInMask(iet->iet_CpuNumber, cur))
        kick = TRUE;

    EXEC_SPINLOCK_UNLOCK(&task->tc_SpinLock);
    Enable();

    /* Lost a race with another SetTaskAffinity(); never published */
    if (fresh)
        KrnFreeCPUMask(fresh);

    if (kick)
        KrnScheduleCPU(cur);

    return TRUE;
#else
    (void)task;
    (void)affinity;

    return FALSE;
#endif

    AROS_LIBFUNC_EXIT

} /* SetTaskAffinity */
