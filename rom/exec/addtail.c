/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.9  1997/01/01 03:46:04  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.8  1996/12/10 13:51:36  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.7  1996/10/24 15:50:42  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.6  1996/10/21 20:46:42  aros
    Changed struct SysBase to struct ExecBase

    Revision 1.5  1996/08/13 13:55:56  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.4  1996/08/01 17:41:03  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"
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

    HISTORY
	26-08-95    digulla created after EXEC-Routine
	26-10-95    digulla adjusted to new calling scheme

******************************************************************************/
{
    AROS_LIBFUNC_INIT
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
    AROS_LIBFUNC_EXIT
} /* AddTail */
