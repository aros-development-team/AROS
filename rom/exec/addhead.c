/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1996/10/24 15:50:41  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.6  1996/10/21 20:46:11  aros
    Changed struct SysBase to struct ExecBase

    Revision 1.5  1996/08/13 13:55:55  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.4  1996/08/01 17:41:01  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/lists.h>
	#include <clib/exec_protos.h>

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
    AROS_LIBFUNC_INIT
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
    AROS_LIBFUNC_EXIT
} /* AddHead */

