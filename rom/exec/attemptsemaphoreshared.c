/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:06  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:39  aros
    Moved all #include's in the first column so makedepend can see it.

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

