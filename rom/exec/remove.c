/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a node from a list
    Lang: english
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <exec/lists.h>
#include <proto/exec.h>

	AROS_LH1I(void, Remove,

/*  SYNOPSIS */
	AROS_LHA(struct Node *, node, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 42, Exec)

/*  FUNCTION
	Remove a node from a list.

    INPUTS
	node - This node to be removed.

    RESULT

    NOTES
	There is no need to specify the list but the node must be in
	a list !

    EXAMPLE
	struct Node * node;

	// Remove node
	Remove (node);

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	26-08-95    digulla created after EXEC-Routine
	26-10-95    digulla adjusted to new calling scheme

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
	Unfortunately, there is no (quick) check that the node
	is in a list.
    */
    ASSERT(node != NULL);

    /*
	Just bend the pointers around the node, ie. we make our
	predecessor point to our successor and vice versa
    */
    node->ln_Pred->ln_Succ = node->ln_Succ;
    node->ln_Succ->ln_Pred = node->ln_Pred;
    AROS_LIBFUNC_EXIT
} /* Remove */

