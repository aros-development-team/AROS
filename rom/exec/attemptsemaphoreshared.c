/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Try to lock a semaphore shared.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, AttemptSemaphoreShared,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 120, Exec)

/*  FUNCTION
	Tries to get a shared lock on a signal semaphore. If the lock cannot
	be obtained false is returned. There may be more than one shared lock
	at a time but an exclusive lock prevents all other locks. The lock
	must be released with ReleaseSemaphore().

    INPUTS
	sigSem - pointer to semaphore structure

    RESULT
	True if the semaphore could be obtained, false otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ReleaseSemaphore()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    exec_lib.fd and clib/exec_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)
    LONG ret=0;

    /* Arbitrate for the semaphore structure */
    Forbid();

    /*
	If the semaphore is free, shared locked or owned by the current task
	it's possible to get another lock.
    */
    if(sigSem->ss_NestCount<=0||sigSem->ss_Owner==SysBase->ThisTask)
    {
	/* Get it and return success */
	ObtainSemaphoreShared(sigSem);
	ret=1;
    }

    /* All done. */
    Permit();
    return ret;
    AROS_LIBFUNC_EXIT
} /* AttemptSemaphoreShared */

