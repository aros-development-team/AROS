/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Delete a memory pool including all its memory.
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include "memory.h"
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, DeletePool,

/*  SYNOPSIS */
	AROS_LHA(APTR, poolHeader, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 117, Exec)

/*  FUNCTION
	Delete a pool including all it's memory.

    INPUTS
	poolHeader - The pool allocated with CreatePool() or NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePool(), AllocPooled(), FreePooled()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Pool *pool = poolHeader + MEMHEADER_TOTAL;

    /* It is legal to DeletePool(NULL) */
    if(pool != NULL)
    {
    	/* Avoid casts */
	struct Node *p;
	
	/*
	 * Free the list of puddles.
	 * Remember that initial puddle containing the pool structure is also in this list.
	 * We do not need to free it until everything else is freed.
	 */
	for (p = pool->PuddleList.mlh_Head; p->ln_Succ; p = p->ln_Succ)
    	{
    	    if (p != poolHeader)
    	    	FreeMemHeader(p, SysBase);
	}

	/* Free the last puddle, containing the pool header */
	FreeMemHeader(poolHeader, SysBase);
    }
    
    AROS_LIBFUNC_EXIT
    
} /* DeletePool */

