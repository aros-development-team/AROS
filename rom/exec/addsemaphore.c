/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:42  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:55:56  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:03  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/semaphores.h>
	#include <clib/exec_protos.h>

	AROS_LH1(void, AddSemaphore,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 100, Exec)

/*  FUNCTION
	Adds a semaphore to the system public semaphore list. Since the
	semaphore gets initialized by this function it must be free at
	this time. Also the ln_Name field must be set.

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

    /* Intialize semaphore */
    sigSem->ss_Link.ln_Type=NT_SIGNALSEM;
    InitSemaphore(sigSem);

    /* Arbitrate for the semaphore list */
    Forbid();
    /* Add the semaphore */
    Enqueue(&SysBase->SemaphoreList,&sigSem->ss_Link);

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* AddSemaphore */

