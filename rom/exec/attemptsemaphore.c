/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:05  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/semaphores.h>
	#include <clib/exec_protos.h>

	__AROS_LH1(ULONG, AttemptSemaphore,

/*  SYNOPSIS */
	__AROS_LA(struct SignalSemaphore *, sigSem, A0),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct ExecBase *,SysBase)

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

    __AROS_FUNC_EXIT
} /* AttemptSemaphore */

