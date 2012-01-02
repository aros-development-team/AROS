/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a node at the end of a list.
    Lang: english
*/
#include <aros/debug.h>
#include <exec/lists.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2I(void, AddTail,

/*  SYNOPSIS */
	AROS_LHA(struct List *, list, A0),
	AROS_LHA(struct Node *, node, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 41, Exec)

/*  FUNCTION
	Insert Node node at the end of a list.

    INPUTS
	list - The list to insert the node into
	node - This node is to be inserted

    RESULT

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * pred;

	// Insert Node at end of the list
	AddTail (list, node);

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
//  ASSERT_VALID_PTR(node); argh! TypeOfMem() doesn't know about the data segment!
//  ASSERT_VALID_PTR(list); argh! Infinite loop in TypeOfMem() if called from ObtainSemaphoreShared() in FindMem()!

    /*
	Make the node point to the head of the list. Our predecessor is the
	previous last node of the list.
    */
    node->ln_Succ	       = (struct Node *)&list->lh_Tail;
    node->ln_Pred	       = list->lh_TailPred;

    /*
	Now we are the last now. Make the old last node point to us
	and the pointer to the last node, too.
    */
    list->lh_TailPred->ln_Succ = node;
    list->lh_TailPred	       = node;
    AROS_LIBFUNC_EXIT
} /* AddTail */
