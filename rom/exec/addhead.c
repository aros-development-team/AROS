/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:27:02  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/lists.h>
	#include <clib/exec_protos.h>

	__AROS_LH2I(void, AddHead,

/*  SYNOPSIS */
	__AROS_LA(struct List *, list, A0),
	__AROS_LA(struct Node *, node, A1),

/*  LOCATION */
	struct SysBase *, SysBase, 40, Exec)

/*  FUNCTION
	Insert Node node as the first node of the list.

    INPUTS
	list - The list to insert the node into
	node - This node is to be inserted

    RESULT

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * pred;

	// Insert Node at top
	AddHead (list, node);

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	26-08-95    digulla created after EXEC-Routine
	26-10-95    digulla adjusted to new calling scheme

******************************************************************************/
{
    __AROS_FUNC_INIT
    assert (node);
    assert (list);

    /*
	Make the node point to the old first node in the list and to the
	head of the list.
    */
    node->ln_Succ	   = list->lh_Head;
    node->ln_Pred	   = (struct Node *)&list->lh_Head;

    /*
	New we come before the old first node which must now point to us
	and the same applies to the pointer to-the-first-node in the
	head of the list.
    */
    list->lh_Head->ln_Pred = node;
    list->lh_Head	   = node;
    __AROS_FUNC_EXIT
} /* AddHead */

