/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:09  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:44  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:49  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:02  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:11  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(struct SignalSemaphore *, FindSemaphore,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, A1),

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
    AROS_LIBFUNC_INIT

    /* Nothing spectacular - just look into the list */
    return (struct SignalSemaphore *)FindName(&SysBase->SemaphoreList,name);
    AROS_LIBFUNC_EXIT
} /* FindSemaphore */

