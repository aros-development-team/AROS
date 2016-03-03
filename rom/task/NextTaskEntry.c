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

        AROS_LH2(struct Task *, NextTaskEntry,

/*  SYNOPSIS */
        AROS_LHA(struct TaskList *, tlist, D1),
        AROS_LHA(ULONG           , flags, D2),

/*  LOCATION */
	struct TaskResBase *, TaskResBase, 3, Task)

/*  FUNCTION
        Looks for the next task list entry with the right type. The list
        must be locked for this.

    INPUTS
        tlist - the value given by LockTaskList()
        flags - the same flags as given to LockTaskList() or a subset
                of them.

    RESULT
        Pointer to task entry found or NULL if the are no more entries.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        LockTaskList(), UnLockTaskList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TaskListPrivate *taskList = (struct TaskListPrivate *)tlist;
    struct Task *retVal = NULL;

    D(bug("[TaskRes] NextTaskEntry: tlist @ 0x%p, flags = $%lx\n", tlist, flags));

    if (taskList)
    {
        if ((taskList->tlp_Flags & ~LTF_WRITE) != 0)
        {
            if (taskList->tlp_Flags & LTF_RUNNING)
            {
                while (taskList->tlp_Next &&
                           ((!taskList->tlp_Next->tle_Task) ||
                            (taskList->tlp_Next->tle_Task->tc_State != TS_RUN)))
                {
                     taskList->tlp_Next = (struct TaskListEntry *)GetSucc(taskList->tlp_Next);
                }
            }
            else if (taskList->tlp_Flags & LTF_READY)
            {
                while (taskList->tlp_Next &&
                           ((!taskList->tlp_Next->tle_Task) ||
                            (taskList->tlp_Next->tle_Task->tc_State != TS_READY)))
                {
                     taskList->tlp_Next = (struct TaskListEntry *)GetSucc(taskList->tlp_Next);
                }
            }
            else if (taskList->tlp_Flags & LTF_WAITING)
            {
                while (taskList->tlp_Next && 
                          ((!taskList->tlp_Next->tle_Task) ||
                          !((taskList->tlp_Next->tle_Task->tc_State == TS_WAIT) ||
                            (taskList->tlp_Next->tle_Task->tc_State == TS_SPIN))))
                {
                     taskList->tlp_Next = (struct TaskListEntry *)GetSucc(taskList->tlp_Next);
                }
            }
        }

        if (taskList->tlp_Next)
        {
            retVal = taskList->tlp_Next->tle_Task;
            taskList->tlp_Next = (struct TaskListEntry *)GetSucc(taskList->tlp_Next);
        }
    }

    return retVal;

    AROS_LIBFUNC_EXIT
} /* NextTaskEntry */
