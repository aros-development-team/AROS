/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:27:15  digulla
    Added copyright notics and made headers conform

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

	__AROS_LH1I(struct Node *, RemHead,

/*  SYNOPSIS */
	__AROS_LA(struct List *, list, A0),

/*  LOCATION */
	struct SysBase *, SysBase, 43, Exec)

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
    __AROS_FUNC_EXIT
} /* RemHead */

