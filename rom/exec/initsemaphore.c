/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:56:03  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
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

	__AROS_LH1I(void, InitSemaphore,

/*  SYNOPSIS */
	__AROS_LHA(struct SignalSemaphore *, sigSem, A0),

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
    __AROS_FUNC_INIT

    /* Clear list of wait messages */
    sigSem->ss_WaitQueue.mlh_Head    =(struct MinNode *)&sigSem->ss_WaitQueue.mlh_Tail;
    sigSem->ss_WaitQueue.mlh_Tail    =NULL;
    sigSem->ss_WaitQueue.mlh_TailPred=(struct MinNode *)&sigSem->ss_WaitQueue.mlh_Head;

    /* Semaphore is currently unused */
    sigSem->ss_NestCount=0;

    __AROS_FUNC_EXIT
} /* InitSemaphore */

