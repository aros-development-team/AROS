/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 16:37:01  digulla
    Replaced hacks with commented versions

 * Revision 1.1  1995/11/05  22:49:09  digulla
 * Initial revision
 *
    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/lists.h>
	#include <clib/exec_protos.h>

	__AROS_LH2I(void, AddTail,

/*  SYNOPSIS */
	__AROS_LA(struct List *, list, A0),
	__AROS_LA(struct Node *, node, A1),

/*  LOCATION */
	struct SysBase *, SysBase, 41, Exec)

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

    HISTORY
	26-08-95    digulla created after EXEC-Routine
	26-10-95    digulla adjusted to new calling scheme

******************************************************************************/
{
    __AROS_FUNC_INIT
    assert (node);
    assert (list);

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
    __AROS_FUNC_EXIT
} /* AddTail */
