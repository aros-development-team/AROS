/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove the first node of a list
    Lang: english
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <exec/lists.h>
#include <proto/exec.h>

	AROS_LH1I(struct Node *, RemHead,

/*  SYNOPSIS */
	AROS_LHA(struct List *, list, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 43, Exec)

/*  FUNCTION
	Remove the first node from a list.

    INPUTS
	list - Remove the node from this list

    RESULT
	The node that has been removed.

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * head;

	// Remove node and return it
	head = RemHead (list);

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Node * node;

    ASSERT(list != NULL);
    /*
	Unfortunately, there is no (quick) check that the node
	is in a list
    */

    /* Get the address of the first node or NULL */
    node = list->lh_Head->ln_Succ;
    if (node)
    {
	node->ln_Pred = (struct Node *)list;
	node = list->lh_Head;
	list->lh_Head = node->ln_Succ;
    }

    /* Return the address or NULL */
    return node;
    AROS_LIBFUNC_EXIT
} /* RemHead */

