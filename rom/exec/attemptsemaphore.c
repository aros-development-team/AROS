/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Try to lock a sempahore.
    Lang: english
*/
#include "exec_intern.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

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

