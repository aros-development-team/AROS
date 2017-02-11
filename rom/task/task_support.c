/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>

#include <proto/exec.h>

#include "task_intern.h"

void task_CleanList(struct Task * task, struct TaskResBase *TaskResBase)
{
    /* Are there any lock holders?, if not, do housecleaning */
    if (IsListEmpty(&TaskResBase->trb_LockedLists))
    {
        struct TaskListEntry *taskEntry, *tetmp;

#if !defined(__AROSEXEC_SMP__)
        /* Don't let any other task interfere with us at the moment */
        Forbid();
#else
        EXEC_SPINLOCK_LOCK(&TaskResBase->TaskListSpinLock, SPINLOCK_MODE_WRITE);
#endif

        ForeachNodeSafe(&TaskResBase->trb_TaskList, taskEntry, tetmp)
        {
            if ((!taskEntry->tle_Task) ||
                ((task) && (task == taskEntry->tle_Task)))
            {
                D(bug("[TaskRes] RemTask: destroying old taskentry @ 0x%p\n", taskEntry));
                Remove(&taskEntry->tle_Node);
                FreeMem(taskEntry, sizeof(struct TaskListEntry));
            }
        }
        ForeachNodeSafe(&TaskResBase->trb_NewTasks, taskEntry, tetmp)
        {
            Remove(&taskEntry->tle_Node);
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }

#if !defined(__AROSEXEC_SMP__)
        Permit();
#else
        EXEC_SPINLOCK_UNLOCK(&TaskResBase->TaskListSpinLock);
#endif
    }
}
