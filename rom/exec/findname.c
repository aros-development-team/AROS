/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1996/10/21 20:47:33  aros
    Changed struct SysBase to struct ExecBase

    Revision 1.7  1996/10/19 17:17:44  aros
    Use the ANSI C function

    Revision 1.6  1996/09/12 13:23:23  digulla
    Fixed a severe bug in the code. If nothing was found, the function returned
	the list-header instead of NULL

    Revision 1.5  1996/08/13 13:56:01  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.4  1996/08/01 17:41:10  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include "exec_intern.h"
#include <aros/libcall.h>
#include <string.h>

/*****************************************************************************

    NAME */
	#include <exec/lists.h>
	#include <clib/exec_protos.h>

	__AROS_LH2I(struct Node *, FindName,

/*  SYNOPSIS */
	__AROS_LHA(struct List *, list, A0),
	__AROS_LHA(UBYTE       *, name, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 46, Exec)

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
    for (node=GetHead(list); node; node=GetSucc(node))
    {
	/* check the node. If we found it, stop */
	if (!strcmp (node->ln_Name, name))
	    break;
    }

    /*
	If we found a node, this will contain the pointer to it. If we
	didn't, this will be NULL (either because the list was
	empty or because we tried all nodes in the list)
    */
    return node;
    __AROS_FUNC_EXIT
} /* FindName */

