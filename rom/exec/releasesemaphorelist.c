/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release all semaphores in the list.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, ReleaseSemaphoreList,

/*  SYNOPSIS */
	AROS_LHA(struct List *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 98, Exec)

/*  FUNCTION
	This function releases all semaphores in the list at once.

    INPUTS
	sigSem - pointer to list full of semaphores

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SignalSemaphore *ss;

    /*
     *	We own all the semaphores, so just go over the list and release
     *	them one at a time.
     */

    ForeachNode(sigSem, ss)
    {
	ReleaseSemaphore(ss);
    }

    AROS_LIBFUNC_EXIT
} /* ReleaseSemaphoreList */
