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
#define AROS_ALMOST_COMPATIBLE
#include "exec_intern.h"
#include <aros/libcall.h>
#include <clib/aros_protos.h>

/*****************************************************************************

    NAME */
	#include <exec/lists.h>
	#include <clib/exec_protos.h>

	__AROS_LH2I(struct Node *, FindName,

/*  SYNOPSIS */
	__AROS_LA(struct List *, list, A0),
	__AROS_LA(UBYTE       *, name, A1),

/*  LOCATION */
	struct SysBase *, SysBase, 46, Exec)

/*  FUNCTION
	Look for a node with a certain name in a list.

    INPUTS
	list - Search this list.
	name - This is the name to look for.

    RESULT

    NOTES
	The search is case-sensitive, so "Hello" will not find a node
	named "hello".

	The list must contain complete Nodes and no MinNodes.

    EXAMPLE
	struct List * list;
	struct Node * node;

	// Look for a node with the name "Hello"
	node = FindName (list, "Hello");

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	26-08-95    digulla created after EXEC-Routine
	26-10-95    digulla adjusted to new calling scheme

******************************************************************************/
{
    __AROS_FUNC_INIT
    struct Node * node;

    assert (list);
    assert (name);

    /* Look through the list */
    node = list->lh_Head;

    while (node->ln_Succ != NULL)
    {
	/* check the node. If we found it, stop */
	if (!STRCMP (node->ln_Name, name))
	    break;

	node = node->ln_Succ;
    }

    /*
	If we found a node, this will contain the pointer to it. If we
	didn't, this will be NULL (either because the list was
	empty or because we tried all nodes in the list)
    */
    return node;
    __AROS_FUNC_EXIT
} /* FindName */

