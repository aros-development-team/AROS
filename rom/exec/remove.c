/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1996/12/10 13:51:52  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.7  1996/10/24 15:50:56  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.6  1996/10/21 20:48:22  aros
    Changed struct SysBase to struct ExecBase

    Revision 1.5  1996/08/13 13:56:07  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.4  1996/08/01 17:41:17  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <exec/lists.h>
#include <clib/exec_protos.h>

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
    AROS_LIBFUNC_EXIT
} /* Remove */

