/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Search for a node by name.
    Lang: english
*/
#include <string.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <exec/lists.h>
#include <proto/exec.h>

	AROS_LH2I(struct Node *, FindName,

/*  SYNOPSIS */
	AROS_LHA(struct List *, list, A0),
	AROS_LHA(const UBYTE *, name, A1),

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

	When supplied with a NULL list argument, defaults to the exec port list.

    EXAMPLE
	struct List * list;
	struct Node * node;

	// Look for a node with the name "Hello"
	node = FindName (list, "Hello");

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Node * node;
/* FIX !
	FindName supplied with a NULL list defaults to the exec port list
	Changed in lists.c as well....
*/
    if( !list )
        list = &SysBase->PortList;

/*    ASSERT(list != NULL); */
    ASSERT(name);

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

