/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:15  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:53  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:56  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:07  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:17  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/

#include "exec_intern.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, RemSemaphore,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 101, Exec)

/*  FUNCTION
	Removes a semaphore from the system public semaphore list.

    INPUTS
	sigSem - Pointer to semaphore structure

    RESULT

    NOTES
	Semaphores are shared between the tasks that use them and must
	therefore lie in public (or at least shared) memory.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Arbitrate for the semaphore list */
    Forbid();

    /* Remove the semaphore */
    Remove(&sigSem->ss_Link);

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* RemSemaphore */

