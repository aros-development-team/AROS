/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:27:15  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/lists.h>
	#include <clib/exec_protos.h>

	__AROS_LH1I(void, Remove,

/*  SYNOPSIS */
	__AROS_LA(struct Node *, node, A1),

/*  LOCATION */
	struct SysBase *, SysBase, 42, Exec)

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
    __AROS_FUNC_INIT
    assert (node);
    /*
	Unfortunately, there is no (quick) check that the node
	is in a list.
    */

    /*
	Just bend the pointers around the node, ie. we make our
	predecessor point to out successor and vice versa
    */
    node->ln_Pred->ln_Succ = node->ln_Succ;
    node->ln_Succ->ln_Pred = node->ln_Pred;
    __AROS_FUNC_EXIT
} /* Remove */

