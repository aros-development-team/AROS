/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/10/21 20:47:07  aros
    Changed struct SysBase to struct ExecBase

    Revision 1.5  1996/08/13 13:56:01  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.4  1996/08/01 17:41:10  digulla
    Added standard header for all files

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

	__AROS_LH2I(void, Enqueue,

/*  SYNOPSIS */
	__AROS_LHA(struct List *, list, A0),
	__AROS_LHA(struct Node *, node, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 45, Exec)

/*  FUNCTION
	Sort a node into a list. The sort-key the field node->ln_Pri.

    INPUTS
	list - Insert into this list. The list has to be in descending
		order in respect to the field ln_Pri of all nodes.
	node - This node is to be inserted. Note that this has to
		be a complete node and not a MinNode !

    RESULT

    NOTES
	The list has to be in descending order in respect to the field
	ln_Pri of all nodes.

    EXAMPLE
	struct List * list;
	struct Node * node;

	// Sort the node at the correct place into the list
	Enqueue (list, node);

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	26-08-95    digulla created after EXEC-Routine
	26-10-95    digulla adjusted to new calling scheme

******************************************************************************/
{
    __AROS_FUNC_INIT
    struct Node * next;

    assert (list);
    assert (node);

    /* Look through the list */
    for (next=GetHead(list); next; next=GetSucc(next))
    {
	/*
	    if the NEXT node has lower prio than the new node, insert us
	    before the next node
	*/
	if (node->ln_Pri >= next->ln_Pri)
	{
	    /* Same as insert but insert before instead of insert behind */
	    node->ln_Succ = next;
	    node->ln_Pred = next->ln_Pred;

	    next->ln_Pred->ln_Succ = node;
	    next->ln_Pred	   = node;

	    /*
		Done. We cannot simly break the loop because of the AddTail()
		below.
	    */
	    return;
	}
    }

    /*
	If no nodes were in the list or our node has the lowest prio,
	we add it as last node
    */
    node->ln_Succ	       = (struct Node *)&list->lh_Tail;
    node->ln_Pred	       = list->lh_TailPred;

    list->lh_TailPred->ln_Succ = node;
    list->lh_TailPred	       = node;
    __AROS_FUNC_EXIT
} /* Enqueue */

