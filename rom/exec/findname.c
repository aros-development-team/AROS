/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.12  1997/02/28 00:14:20  ldp
    Check if a node has a name, and only strcmp() if it has. Should fix streams
    of Enforcer hits of bytereads from 0. I didn't get them, but I got hitlogs of
    35kB, so they sure were there. :)

    Revision 1.11  1997/01/01 03:46:09  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.10  1996/12/10 13:51:44  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.9  1996/10/24 15:50:48  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.8  1996/10/21 20:47:33  aros
    Changed struct SysBase to struct ExecBase

    Revision 1.7  1996/10/19 17:17:44  aros
    Use the ANSI C function

    Revision 1.6  1996/09/12 13:23:23  digulla
    Fixed a severe bug in the code. If nothing was found, the function returned
	the list-header instead of NULL

    Revision 1.5  1996/08/13 13:56:01  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
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
#include <exec/lists.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2I(struct Node *, FindName,

/*  SYNOPSIS */
	AROS_LHA(struct List *, list, A0),
	AROS_LHA(UBYTE       *, name, A1),

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
    AROS_LIBFUNC_INIT
    struct Node * node;

    assert (list);
    assert (name);

    /* Look through the list */
    for (node=GetHead(list); node; node=GetSucc(node))
    {
	/* Only compare the names if this node has one. */
	if(node->ln_Name)
	{
	    /* Check the node. If we found it, stop. */
	    if (!strcmp (node->ln_Name, name))
		break;
	}
    }

    /*
	If we found a node, this will contain the pointer to it. If we
	didn't, this will be NULL (either because the list was
	empty or because we tried all nodes in the list)
    */
    return node;
    AROS_LIBFUNC_EXIT
} /* FindName */

