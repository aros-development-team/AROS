/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:56:06  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:16  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"

/*****************************************************************************

    NAME */
	#include <exec/semaphores.h>
	#include <clib/exec_protos.h>

	__AROS_LH1(void, ReleaseSemaphoreList,

/*  SYNOPSIS */
	__AROS_LHA(struct List *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 98, Exec)

/*  FUNCTION
	This function releases all semaphores in the list at once.

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
    __AROS_FUNC_INIT
    struct Node *n;

    /*
	There's no arbitration needed - the first semaphore in the list
	arbitrates for the full list.
	Get first element in the list.
    */
    n=sigSem->lh_Head;

    /* And follow it. */
    while(n->ln_Succ!=NULL)
    {
	/* Free the semaphore */
	ObtainSemaphore((struct SignalSemaphore *)n);

	/* Get next element */
	n=n->ln_Succ;
    }
    __AROS_FUNC_EXIT
} /* ReleaseSemaphoreList */
