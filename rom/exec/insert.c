/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include <exec/lists.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH3I(void, Insert,

/*  SYNOPSIS */
	AROS_LHA(struct List *, list, A0),
	AROS_LHA(struct Node *, node, A1),
	AROS_LHA(struct Node *, pred, A2),

/*  LOCATION */
	struct ExecBase *, SysBase, 39, Exec)

/*  FUNCTION
	Insert Node node after pred in list.

    INPUTS
	list - The list to insert the node into
	node - This node is to be inserted
	pred - Insert after this node. If this is NULL, node is inserted
		as the first node (same as AddHead()).

    RESULT

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * pred, * node;

	// Insert Node node as second node in list
	pred = GetHead (list);
	Insert (list, node, pred);

    BUGS

    SEE ALSO
	AddHead(), AddTail(), Enqueue(), RemHead(), Remove(), RemTail(),
	"AROS: Exec Lists".

    INTERNALS

    HISTORY
	26-08-95    digulla created after EXEC-Routine
	26-10-95    digulla adjusted to new calling scheme

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    assert (node);
    assert (list);

    /* If we have a node to insert behind... */
    if (pred)
    {
	/*
	    Our successor is the successor of the node we add ourselves
	    behind and our predecessor is just the node itself.
	*/
	node->ln_Succ = pred->ln_Succ;
	node->ln_Pred = pred;

	/*
	    We are the predecessor of the successor of our predecessor
	    (What ? blblblb... ;) and of our predecessor itself.
	    Note that here the sequence is quite important since
	    we need ln_Succ in the first expression and change it in
	    the second.
	*/
	pred->ln_Succ->ln_Pred = node;
	pred->ln_Succ = node;
    }
    else
    {
	/*
	    add at the top of the list. I do not use AddHead() here but
	    write the code twice for two reasons: 1. The code is small and
	    quite prone to errors and 2. If I would call AddHead(), it
	    would take almost as long to call the function as the execution
	    would take yielding 100% overhead.
	*/
	node->ln_Succ	       = list->lh_Head;
	node->ln_Pred	       = (struct Node *)&list->lh_Head;
	list->lh_Head->ln_Pred = node;
	list->lh_Head	       = node;
    }
    AROS_LIBFUNC_EXIT
} /* Insert */

