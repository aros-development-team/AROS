/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize a SignalSemaphore
    Lang: english
*/

#include "exec_intern.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Clear list of wait messages */
    sigSem->ss_WaitQueue.mlh_Head     = (struct MinNode *)&sigSem->ss_WaitQueue.mlh_Tail;
    sigSem->ss_WaitQueue.mlh_Tail     = NULL;
    sigSem->ss_WaitQueue.mlh_TailPred = (struct MinNode *)&sigSem->ss_WaitQueue.mlh_Head;

    /* Set type of Semaphore */
    sigSem->ss_Link.ln_Type = NT_SIGNALSEM;

    /* Semaphore is currently unused */
    sigSem->ss_NestCount = 0;

    /* Semaphore has no owner yet */
    sigSem->ss_Owner = 0;

    /* Semaphore has no queue */
    sigSem->ss_QueueCount = -1;

    AROS_LIBFUNC_EXIT
} /* InitSemaphore */

