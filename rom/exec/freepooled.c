/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free memory allocated by AllocPooled().
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include "memory.h"
#include <exec/memory.h>
#include <proto/exec.h>


/*****************************************************************************

    NAME */

	AROS_LH3(void,FreePooled,

/*  SYNOPSIS */
	AROS_LHA(APTR, poolHeader,A0),
	AROS_LHA(APTR, memory,    A1),
	AROS_LHA(ULONG,memSize,   D0),

/* LOCATION */
	struct ExecBase *, SysBase, 119, Exec)

/*  FUNCTION
	Free memory allocated out of a private memory pool.

    INPUTS
	poolHeader - Handle of the memory pool
	memory	   - Pointer to the memory
	memSize    - Size of the memory chunk

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePool(), DeletePool(), AllocPooled()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct ProtectedPool *pool = (struct ProtectedPool *)poolHeader;

    if (!memory || !memSize) return;
    
    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ObtainSemaphore(&pool->sem);
    }
    
    /* If memSize is bigger than the ThreshSize it's allocated seperately. */
    if(memSize > pool->pool.ThreshSize)
    {
	struct Block *bl;

	/* Get pointer to header */
	bl = (struct Block *)((UBYTE *)memory - BLOCK_TOTAL);

	/* Remove it from the list */
	Remove((struct Node *)&bl->Node);

	if (bl->Size != memSize + BLOCK_TOTAL)
	{
	    kprintf("\nFreePooled: free size does not match alloc size: allocsize = %d freesize = %d!!!\n\n",
	    	       bl->Size - BLOCK_TOTAL,
		       memSize);
	}
	
	/* And Free the memory */
	FreeMem(bl, bl->Size);

    }
    else
    {
	/* Look for the right MemHeader */
	struct MemHeader *mh = (struct MemHeader *)pool->pool.PuddleList.mlh_Head;

	for(;;)
	{
	    /* The memory must be between the two borders */
	    if(memory >= mh->mh_Lower && memory < mh->mh_Upper)
	    {
		/* Found the MemHeader. Free the memory. */
		Deallocate(mh, memory, memSize);

		/* Is this MemHeader completely free now? */
		if(mh->mh_Free == pool->pool.PuddleSize)
		{
		    /* Yes. Remove it from the list. */
		    Remove(&mh->mh_Node);

		    /* And free it. */
		    FreeMem(mh, pool->pool.PuddleSize + MEMHEADER_TOTAL);
		}
		/* All done. */
		break;
	    }
	    /* Try next MemHeader */
	    mh = (struct MemHeader *)mh->mh_Node.ln_Succ;
	}
    }

    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ReleaseSemaphore(&pool->sem);
    }
 
    AROS_LIBFUNC_EXIT
    
} /* FreePooled */

