/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove the last node of a list
    Lang: english
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <exec/lists.h>
#include <proto/exec.h>

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

    /*
	Unfortunately, there is no (quick) check that the node
	is in a list.
    */
    ASSERT(list != NULL);

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

