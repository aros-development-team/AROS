/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:47  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:51  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:03  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:12  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <exec/semaphores.h>
#include <clib/exec_protos.h>

	AROS_LH1I(void, InitSemaphore,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 93, Exec)

/*  FUNCTION
	Prepares a semaphore structure for use by the exec semaphore system,
	i.e. this function must be called after allocating the semaphore and
	before using it or the semaphore functions will fail.

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
	29-10-95    digulla automatically created from
			    exec_lib.fd and clib/exec_protos.h
	21-01-96    fleischer implementation

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Clear list of wait messages */
    sigSem->ss_WaitQueue.mlh_Head    =(struct MinNode *)&sigSem->ss_WaitQueue.mlh_Tail;
    sigSem->ss_WaitQueue.mlh_Tail    =NULL;
    sigSem->ss_WaitQueue.mlh_TailPred=(struct MinNode *)&sigSem->ss_WaitQueue.mlh_Head;

    /* Semaphore is currently unused */
    sigSem->ss_NestCount=0;

    AROS_LIBFUNC_EXIT
} /* InitSemaphore */

