/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:11  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(struct SignalSemaphore *, FindSemaphore,

/*  SYNOPSIS */
	__AROS_LA(STRPTR, name, A1),

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

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /* Nothing spectacular - just look into the list */
    return (struct SignalSemaphore *)FindName(&SysBase->SemaphoreList,name);
    __AROS_FUNC_EXIT
} /* FindSemaphore */

