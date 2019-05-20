/*
    Copyright © 2002-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>

#include "security_intern.h"
#include "security_task.h"

/*****************************************************************************

    NAME */
	AROS_LH3(LONG, secEnumChildren,

/*  SYNOPSIS */
	/* (parent, children, size) */
	AROS_LHA(struct Task *, parent, A0),
	AROS_LHA(struct Task **, children, A1),
	AROS_LHA(LONG, size, D0),

/*  LOCATION */
	struct SecurityBase *, secBase, 36, Security)

/*  FUNCTION
	Enumerate the children of a given task.

    INPUTS
	parent - the Task we are interested in (and may be NULL -> calling task),
	children - an array we should populate
	size - the size of the supplied array (children)

    RESULT
	If the size is too small, we return -(num children) to indicate the size of
	the buffer needed for a successful call.
	This means that a program could call us with a size of -1 to ask us how big
	the buffer should be.

    NOTES
	This is designed to replace secGetChildren/secFreeTaskVec.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTaskNode *node;
    struct MinNode *tempnode;
    LONG result = 0;
    int i = 0;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ( (node = FindTaskNode(secBase, 
            parent ? parent : FindTask(NULL)
            )) )
    {
        if (node->ChildrenCount == 0)
            goto leave;

        /* Allow a program to ask how big a buffer to use */
        if ((node->ChildrenCount > size) || (children == NULL))	{
            result = - node->ChildrenCount;
            goto leave;
        }
        result = node->ChildrenCount;

        /* Fill in children */
        for (tempnode = node->Children.mlh_Head; i < node->ChildrenCount; i++, tempnode = tempnode->mln_Succ)
            children[i] = ((struct secTaskNode *)((IPTR)tempnode-(IPTR)&((struct secTaskNode *)NULL)->Siblings))->Task;
    }

leave:
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return result;

    AROS_LIBFUNC_EXIT

} /* secEnumChildren */

