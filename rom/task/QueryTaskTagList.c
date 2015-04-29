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

#include "etask.h"

#include "taskres_intern.h"

/*****************************************************************************

    NAME */
#include <proto/task.h>

	AROS_LH2(void, QueryTaskTagList,

/*  SYNOPSIS */
	AROS_LHA(struct Task *, task, A0),
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
	struct TaskResBase *, TaskResBase, 1, Task)

/*  FUNCTION

        Provides information about selected system Task
    
    INPUTS

        Function takes an array of tags. Data is returned for each tag. See
        specific tag description.

    TAGS

        TaskTag_CPUTime - (ULONG) Returns the amount of cpu time a task has used .

    RESULT

        None

    NOTES
    
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem * Tag = NULL;

    
    /* This is the default implementation */
        
    while ((Tag = NextTagItem(&tagList)) != NULL)
    {
        switch(Tag->ti_Tag)
        {
        case(TaskTag_CPUTime):
            Tag->ti_Data = GetIntETask(task)->iet_CpuTime;
            break;
        }
    }

    AROS_LIBFUNC_EXIT
} /* QueryTaskTagList() */

