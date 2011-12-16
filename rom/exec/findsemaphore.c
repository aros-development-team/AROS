/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Search a semaphore by name
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(struct SignalSemaphore *, FindSemaphore,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 99, Exec)

/*  FUNCTION
	Find a semaphore with a given name in the system global semaphore list.
	Note that this call doesn't arbitrate for the list - use Forbid() to
	do this yourself.

    INPUTS
	name - Pointer to name.

    RESULT
	Address of semaphore structure found or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Nothing spectacular - just look into the list */
    return (struct SignalSemaphore *)FindName(&SysBase->SemaphoreList,name);
    AROS_LIBFUNC_EXIT
} /* FindSemaphore */

