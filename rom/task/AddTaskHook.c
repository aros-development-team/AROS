/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/task.h>

#include "etask.h"

#include "task_intern.h"

/*****************************************************************************

    NAME */
#include <proto/task.h>

	AROS_LH2(BOOL, AddTaskHook,

/*  SYNOPSIS */
	AROS_LHA(struct Hook *, tHook, A0),
	AROS_LHA(ULONG, thType, D0),

/*  LOCATION */
	struct TaskResBase *, TaskResBase, 10, Task)

/*  FUNCTION

   
    INPUTS

    TAGS

    RESULT

    NOTES
    
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#ifdef TASKRES_ENABLE
    struct Task *thisTask = FindTask(NULL);

    struct TaskListEntry *thisTEntry;
    struct TaskListHookEntry *typeEntry;
    struct TaskListHookNode *hookEntry;

    thisTEntry = GetTaskEntry(thisTask, TaskResBase);
    if (thisTEntry)
    {
        typeEntry = GetHookTypeEntry(&thisTEntry->tle_HookTypes, thType, TRUE);
        if (typeEntry)
        {
            if (typeEntry->tlhe_Node.ln_Pri >= 0)
            {
                hookEntry = AllocMem(sizeof(struct TaskListHookNode *), MEMF_PUBLIC|MEMF_CLEAR);
                hookEntry->tln_Hook = tHook;
                AddTail(&typeEntry->tlhe_Hooks, &hookEntry->tln_Hook->h_MinNode);
            }
            else
            {
                TaskResHookDispatcher trCallHook = (TaskResHookDispatcher)typeEntry->tlhe_Node.ln_Name;
                if (! trCallHook(tHook))
                    return FALSE;
            }
            return TRUE;
        }
    }
#endif

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* AddTaskHook() */
