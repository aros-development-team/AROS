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

        AROS_LH1(struct TaskList *, LockTaskList,

/*  SYNOPSIS */
        AROS_LHA(ULONG, flags, D1),

/*  LOCATION */
	struct TaskResBase *, TaskResBase, 1, Task)

/*  FUNCTION

    INPUTS
        flags - 

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

    return (struct TaskList *)taskList;

    AROS_LIBFUNC_EXIT
} /* LockTaskList */
