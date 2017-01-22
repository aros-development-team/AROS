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

        AROS_LH1(struct TaskList *, LockTaskList,

/*  SYNOPSIS */
        AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct TaskResBase *, TaskResBase, 1, Task)

/*  FUNCTION

    INPUTS
        flags - 
              LTF_WRITE     Lock The TaskList for writing
                            NB: In general software SHOULDNT
                                need to use this!

              LTF_RUNNING   Lock The TaskList to show running tasks.
              LTF_READY     Lock The TaskList to show ready tasks.
              LTF_WAITING   Lock The TaskList to show waiting/spinning tasks.
              LTF_ALL       Lock The TaskList to show all of the above tasks.

    RESULT
        Handle to the task list. This is not a direct pointer
        to the first list element but to a pseudo element instead.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        UnLockTaskList(), NextTaskEntry()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TaskListPrivate *taskList = NULL;

    D(bug("[TaskRes] LockTaskList: flags = $%lx\n", flags));

#ifdef TASKRES_ENABLE
    if (flags & LTF_WRITE)
        ObtainSemaphore(&TaskResBase->trb_Sem);
    else
        ObtainSemaphoreShared(&TaskResBase->trb_Sem);

    if ((taskList = AllocMem(sizeof(struct TaskListPrivate), MEMF_CLEAR)) != NULL)
    {
        D(bug("[TaskRes] LockTaskList: TaskList @ 0x%p\n", taskList));
        taskList->tlp_Node.ln_Name = (char *)FindTask(NULL);
        taskList->tlp_Flags = flags;
        taskList->tlp_Tasks = &TaskResBase->trb_TaskList;
        taskList->tlp_Next = (struct TaskListEntry *)GetHead(taskList->tlp_Tasks);
        AddTail(&TaskResBase->trb_LockedLists, &taskList->tlp_Node);
    }
#else
    Disable();
    if ((taskList = (struct TaskListPrivate *)AllocVec(sizeof(struct TaskListPrivate), MEMF_PUBLIC)) != NULL)
    {
        if (flags & LTF_RUNNING)
            taskList->tlp_TaskList = NULL;
        else if (flags & LTF_READY)
            taskList->tlp_TaskList = &SysBase->TaskReady;
        else if (flags & LTF_WAITING)
            taskList->tlp_TaskList = &SysBase->TaskWait;
        taskList->tlp_Current = NULL;
    }
#endif /* TASKRES_ENABLE */

    return (struct TaskList *)taskList;

    AROS_LIBFUNC_EXIT
} /* LockTaskList */
