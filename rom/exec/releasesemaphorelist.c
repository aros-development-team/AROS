/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:14  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:51  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:55  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:06  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:16  digulla
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

	AROS_LH1(void, ReleaseSemaphoreList,

/*  SYNOPSIS */
	AROS_LHA(struct List *, sigSem, A0),

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
    AROS_LIBFUNC_INIT
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
    AROS_LIBFUNC_EXIT
} /* ReleaseSemaphoreList */
