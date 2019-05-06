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

	AROS_LH3(BOOL, InitTaskHooks,

/*  SYNOPSIS */
	AROS_LHA(APTR, thDispatcher, A0),
	AROS_LHA(ULONG, thType, D0),
	AROS_LHA(ULONG, thFlags, D1),

/*  LOCATION */
	struct TaskResBase *, TaskResBase, 9, Task)

/*  FUNCTION

   
    INPUTS
            thDispatcher - default dispatcher used to call the hook.
            thType - Task Hook Type for the list.
            thFlags -
                            THF_ROA - Runs a TaskHook immidiately when it is added.
                            THF_IAR - Runs a TaskHook immidiately if the TaskHooks have been run.

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

    thisTEntry = GetTaskEntry(thisTask, TaskResBase);
    if (thisTEntry)
    {
        typeEntry = GetHookTypeEntry(&thisTEntry->tle_HookTypes, thType, TRUE);
        if (typeEntry)
        {
            if (!typeEntry->tlhe_Node.ln_Name)
                typeEntry->tlhe_Node.ln_Name = (APTR)thDispatcher;
            switch (thFlags)
            {
            case THF_ROA:
                typeEntry->tlhe_Node.ln_Pri = -1;
                break;
            case THF_IAR:
                typeEntry->tlhe_Node.ln_Pri = 1;
                break;
            default:
                typeEntry->tlhe_Node.ln_Pri = 0;
                break;
            }
            return TRUE;
        }
    }
#endif

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* InitTaskHooks() */
