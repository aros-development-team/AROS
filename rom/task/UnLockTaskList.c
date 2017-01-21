/*
    Copyright © 2015-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/task.h>

#include <resources/task.h>

#include "task_intern.h"

/*****************************************************************************

    NAME */
#include <proto/task.h>

        AROS_LH2(void, UnLockTaskList,

/*  SYNOPSIS */
        AROS_LHA(struct TaskList *, tlist, A0),
        AROS_LHA(ULONG, flags, D0),

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

#ifdef TASKRES_ENABLE
    struct TaskListPrivate *taskList, *tltmp;
    struct Task *thisTask = FindTask(NULL);
#endif /* TASKRES_ENABLE */

    D(bug("[TaskRes] UnLockTaskList: flags = $%lx\n", flags));

#ifdef TASKRES_ENABLE
    ReleaseSemaphore(&TaskResBase->trb_Sem);

    ForeachNodeSafe(&TaskResBase->trb_LockedLists, taskList, tltmp)
    {
        if (((struct Task *)taskList->tlp_Node.ln_Name == thisTask) &&
            (taskList->tlp_Flags == flags))
        {
            D(bug("[TaskRes] UnLockTaskList: Releasing TaskList @ 0x%p\n", taskList));
            Remove(&taskList->tlp_Node);
            FreeMem(taskList, sizeof(struct TaskListPrivate));
            break;
        }
    }

    /* Purge expired entries from the list... */
    task_CleanList(NULL, TaskResBase);
#else
    Enable();
    FreeVec(tlist);
#endif /* TASKRES_ENABLE */

    AROS_LIBFUNC_EXIT
} /* UnLockTaskList */
