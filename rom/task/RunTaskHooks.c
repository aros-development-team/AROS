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

	AROS_LH2(BOOL, RunTaskHooks,

/*  SYNOPSIS */
	AROS_LHA(APTR, thDispatcher, A0),
	AROS_LHA(ULONG, thType, D0),

/*  LOCATION */
	struct TaskResBase *, TaskResBase, 11, Task)

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
    TaskResHookDispatcher trCallHook = (TaskResHookDispatcher)thDispatcher;
    struct Task *thisTask = FindTask(NULL);
    struct TaskListEntry *thisTEntry;
    struct TaskListHookEntry *typeEntry;
    struct TaskListHookNode *hookEntry;

    thisTEntry = GetTaskEntry(thisTask, TaskResBase);
    if (thisTEntry)
    {
        typeEntry = GetHookTypeEntry(&thisTEntry->tle_HookTypes, thType, FALSE);
        if (typeEntry)
        {
            ForeachNode(&typeEntry->tlhe_Hooks, hookEntry)
            {
                if (! trCallHook(hookEntry->tln_Hook))
                    return FALSE;
            }
            if (typeEntry->tlhe_Node.ln_Pri == 1)
                typeEntry->tlhe_Node.ln_Pri = -1;
            return TRUE;
        }
    }
#endif

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* RunTaskHooks() */
