/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Free child task information on a dead child.
*/
#include "exec_intern.h"
#include "exec_util.h"
#include "etask.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, ChildFree,

/*  SYNOPSIS */
	AROS_LHA(ULONG, tid, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 123, Exec)

/*  FUNCTION
	Clean up after a child process.

    INPUTS
	tid	--  Id of the child to clean up. This is not the same as
		    the Task pointer.

    RESULT
	The child will be freed.

    NOTES
	This function will work correctly only for child tasks that are
	processes created with NP_NotifyOnDeath set to TRUE.

	Calling ChildFree() on a running child is likely to crash your
	system badly.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ETask *et;

    Forbid();
    et = FindChild(tid);
    if(et != NULL)
    {
	Remove((struct Node *)et);

        ExpungeETask(et);
    }
    Permit();
    
    AROS_LIBFUNC_EXIT
} /* ChildFree */
