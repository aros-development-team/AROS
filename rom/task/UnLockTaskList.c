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
        LockTaskList(), NextTaskEntry().

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[TaskRes] UnLockTaskList: flags = $%lx\n", flags));

    AROS_LIBFUNC_EXIT
} /* UnLockTaskList */
