/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Delete a memory pool including all it's memory.
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

    struct Pool *pool = (struct Pool *)poolHeader;

    /* It is legal to DeletePool(NULL) */
    if(pool != NULL)
    {
	struct Block 	*bl;
	void 	    	*p;
	ULONG 	    	size;

	/* Calculate the total size of a puddle including header. */
	size = pool->PuddleSize + MEMHEADER_TOTAL;
	
	/* Free the list of puddles */
	while((p = RemHead((struct List *)&pool->PuddleList)) !=NULL )
	{
	    FreeMem(p, size);
    	}
	
	/* Free the list of single Blocks */
	while((bl = (struct Block *)RemHead((struct List *)&pool->BlockList)) != NULL)
	{
	    FreeMem(bl, bl->Size);
    	}
	
	FreeMem(pool, (pool->Requirements & MEMF_SEM_PROTECTED) ? sizeof(struct ProtectedPool) :
	    	    	    	    	    	    	    	  sizeof(struct Pool));
    }
    
    AROS_LIBFUNC_EXIT
    
} /* DeletePool */

