/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/task.h>

#include <resources/task.h>

#include "taskres_intern.h"

/*****************************************************************************

    NAME */
#include <proto/task.h>

        AROS_LH1(void, UnLockTaskList,

/*  SYNOPSIS */
        AROS_LHA(ULONG, flags, D1),

/*  LOCATION */
	struct TaskResBase *, TaskResBase, 2, Task)

/*  FUNCTION
        Frees a lock on the task lists given by LockTaskList().

    INPUTS
        flags - the same value as given to LockTaskList().

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        LockTaskList(), NextTaskEntry()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TaskListPrivate *taskList, *tltmp;
    struct Task *thisTask = FindTask(NULL);

    D(bug("[TaskRes] UnLockTaskList: flags = $%lx\n", flags));

    ReleaseSemaphore(&TaskResBase->trb_Sem);

    ForeachNodeSafe(&TaskResBase->trb_LockedLists, taskList, tltmp)
    {
        if ((taskList->tlp_Node.ln_Name == (char *)thisTask) &&
            (taskList->tlp_Flags == flags))
        {
            D(bug("[TaskRes] UnLockTaskList: Releasing TaskList @ 0x%p\n", taskList));
            Remove(&taskList->tlp_Node);
            FreeMem(taskList, sizeof(struct TaskListPrivate));
            break;
        }
    }

    /* If we are the last lock holder, do housecleaning */
    if (IsListEmpty(&TaskResBase->trb_LockedLists))
    {
        struct TaskListEntry *taskEntry, *tetmp;

        ForeachNodeSafe(&TaskResBase->trb_TaskList, taskEntry, tetmp)
        {
            if (taskEntry->tle_Task == NULL)
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
    }
    AROS_LIBFUNC_EXIT
} /* UnLockTaskList */
