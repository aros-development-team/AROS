/*
    Copyright © 2017-2019, The AROS Development Team. All rights reserved.
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
        EXEC_SPINLOCK_LOCK(&TaskResBase->TaskListSpinLock, NULL, SPINLOCK_MODE_WRITE);
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

struct TaskListEntry *GetTaskEntry(struct Task *task, struct TaskResBase *TaskResBase)
{
    struct TaskListEntry *currNode, *retval = NULL;

#if !defined(__AROSEXEC_SMP__)
        /* Don't let any other task interfere with us at the moment */
        Forbid();
#else
        EXEC_SPINLOCK_LOCK(&TaskResBase->TaskListSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif

    ForeachNode(&TaskResBase->trb_TaskList, currNode)
    {
        if (currNode->tle_Task == task)
        {
            retval = currNode;
            break;
        }
    }

#if !defined(__AROSEXEC_SMP__)
        Permit();
#else
        EXEC_SPINLOCK_UNLOCK(&TaskResBase->TaskListSpinLock);
#endif

    return retval;
}

/* task hook support */
struct TaskListHookEntry *GetHookTypeEntry(struct List *htList, ULONG thType, BOOL create)
{
    struct TaskListHookEntry *currEntry;

    D(bug("%s(0x%p)\n", __func__, htList));

    ForeachNode(htList, currEntry)
    {
        if (currEntry->tlhe_Node.ln_Type == thType)
        {
            D(bug("Hook Type Entry Found @ 0x%p\n", currEntry));
            return currEntry;
        }
    }

    D(bug("Hook Type %d Not Found\n", thType));

    if (create)
    {
        currEntry = AllocMem(sizeof(struct TaskListHookEntry), MEMF_ANY|MEMF_CLEAR);
        if (currEntry)
        {
            D(bug("New Hook Type Entry @ 0x%p for type %d\n", currEntry, thType));

            currEntry->tlhe_Node.ln_Type = thType;
            NEWLIST(&currEntry->tlhe_Hooks);
            AddTail(htList, &currEntry->tlhe_Node);

            return currEntry;
        }
    }

    return NULL;
}

VOID TaskHookTypeDispose(struct TaskListHookEntry *task, ULONG type)
{
    D(bug("%s(0x%p)\n", __func__));
}

VOID TaskHooksDispose(struct TaskListHookEntry *task)
{
    D(bug("%s(0x%p)\n", __func__));
}
