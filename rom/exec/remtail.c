/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 16:37:02  digulla
    Replaced hacks with commented versions

 * Revision 1.1  1995/11/05  22:49:09  digulla
 * Initial revision
 *
    Desc:
    Lang: english
*/
/* I want the macros */
#define AROS_ALMOST_COMPATIBLE
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/lists.h>
	#include <clib/exec_protos.h>

	__AROS_LH1I(struct Node *, RemTail,

/*  SYNOPSIS */
	__AROS_LA(struct List *, list, A0),

/*  LOCATION */
	struct SysBase *, SysBase, 44, Exec)

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
    __AROS_FUNC_INIT
    struct Node * node;

    assert (list);
    /*
	Unfortunately, there is no (quick) check that the node
	is in a list.
    */

    /* Get the last node of the list */
    if (node = GetTail (list))
    {
	/* normal code to remove a node if there is one */
	node->ln_Pred->ln_Succ = node->ln_Succ;
	node->ln_Succ->ln_Pred = node->ln_Pred;
    }

    /* return it's address or NULL if there was no node */
    return node;
    __AROS_FUNC_EXIT
} /* RemTail */

