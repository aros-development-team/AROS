/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.9  1997/01/01 03:46:15  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.8  1996/12/10 13:51:53  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.7  1996/10/24 15:50:56  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.6  1996/10/21 20:48:22  aros
    Changed struct SysBase to struct ExecBase

    Revision 1.5  1996/08/13 13:56:07  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.4  1996/08/01 17:41:17  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/

/* I want the macros */
#define AROS_ALMOST_COMPATIBLE
#include "exec_intern.h"
#include <exec/lists.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1I(struct Node *, RemTail,

/*  SYNOPSIS */
	AROS_LHA(struct List *, list, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 44, Exec)

/*  FUNCTION
	Remove the last node from a list.

    INPUTS
	list - Remove the node from this list

    RESULT
	The node that has been removed.

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * tail;

	// Remove node and return it
	tail = RemTail (list);

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	26-08-95    digulla created after EXEC-Routine
	26-10-95    digulla adjusted to new calling scheme

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Node * node;

    assert (list);
    /*
	Unfortunately, there is no (quick) check that the node
	is in a list.
    */

    /* Get the last node of the list */
    if ( (node = GetTail (list)) )
    {
	/* normal code to remove a node if there is one */
	node->ln_Pred->ln_Succ = node->ln_Succ;
	node->ln_Succ->ln_Pred = node->ln_Pred;
    }

    /* return it's address or NULL if there was no node */
    return node;
    AROS_LIBFUNC_EXIT
} /* RemTail */

