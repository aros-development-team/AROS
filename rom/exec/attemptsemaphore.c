/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:45  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:55:58  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:05  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/semaphores.h>
	#include <clib/exec_protos.h>

	AROS_LH1(ULONG, AttemptSemaphore,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 96, Exec)

/*  FUNCTION
	Tries to get an exclusive lock on a signal semaphore. If the semaphore
	is already in use by another task this function does not wait but
	return false instead.

    INPUTS
	sigSem - Pointer so semaphore structure.

    RESULT
	TRUE if the semaphore could be obtained, FALSE otherwise.

    NOTES
	The lock must be freed with ReleaseSemaphore().

    EXAMPLE

    BUGS

    SEE ALSO
	ReleaseSemaphore()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    exec_lib.fd and clib/exec_protos.h
	21-01-96    fleischer implementation

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    LONG ret=0;

    /* Arbitrate for semaphore nesting count and owner fields */
    Forbid();

    /* If the semaphore is free or the user is the current task */
    if(!sigSem->ss_NestCount||sigSem->ss_Owner==SysBase->ThisTask)
    {
	/* Get an exclusive lock on it */
	ObtainSemaphore(sigSem);

	/* And return true */
	ret=1;
    }

    /* All done */
    Permit();
    return ret;

    AROS_LIBFUNC_EXIT
} /* AttemptSemaphore */

