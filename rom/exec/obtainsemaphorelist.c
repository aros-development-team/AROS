/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Lock all semaphores in the list at once.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, ObtainSemaphoreList,

/*  SYNOPSIS */
	AROS_LHA(struct List *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 97, Exec)

/*  FUNCTION
	This function obtains all semaphores in the list at once.
	Note that this doesn't include arbitration for the list as
	a whole - you will have to arbitrate for the whole list yourself.

    INPUTS
	sigSem - pointer to list full of semaphores

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Node *n;

#warning !!!!!!!!!! ObtainSemaphoreList must be rewritten !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    /*
	There's no arbitration needed - the first semaphore in the list
	list arbitrates for the full list.
	Get first element in the list.
    */
    n=sigSem->lh_Head;

    /* And follow it. */
    while(n->ln_Succ!=NULL)
    {
	/* Free the semaphore */
	ReleaseSemaphore((struct SignalSemaphore *)n);

	/* Get next element */
	n=n->ln_Succ;
    }
    AROS_LIBFUNC_EXIT
} /* ReleaseSemaphoreList */
