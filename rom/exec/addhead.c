/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a node to the head of a list
    Lang: english
*/
#include <aros/debug.h>
#include <exec/lists.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2I(void, AddHead,

/*  SYNOPSIS */
	AROS_LHA(struct List *, list, A0),
	AROS_LHA(struct Node *, node, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 40, Exec)

/*  FUNCTION
	Insert Node node as the first node of the list.

    INPUTS
	list - The list to insert the node into
	node - This node is to be inserted

    RESULT
	None.

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * pred;

	// Insert Node at top
	AddHead (list, node);

    BUGS

    SEE ALSO
	libamiga/NewList(), AddTail(), Insert(), Remove(), RemHead(), RemTail(),
	Enqueue()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    ASSERT_VALID_PTR(node);
    ASSERT_VALID_PTR(list);

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
    AROS_LIBFUNC_EXIT
} /* AddHead */

