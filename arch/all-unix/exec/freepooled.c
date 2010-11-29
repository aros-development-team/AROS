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
    
    struct ProtectedPool *pool = poolHeader + MEMHEADER_TOTAL;
    struct MemHeader *mh;

    if (!memory || !memSize) return;
    
    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ObtainSemaphore(&pool->sem);
    }
    
    /* Look for the right MemHeader */
    for (mh = (struct MemHeader *)pool->pool.PuddleList.mlh_Head;; mh = (struct MemHeader *)mh->mh_Node.ln_Succ)
    {
        if (!mh->mh_Node.ln_Succ)
	{
	    /* memory block is not in pool. */
	    Alert(AT_Recovery | AN_MemCorrupt);
	    break;
	}

	/* The memory must be between the two borders */
	if(memory >= mh->mh_Lower && memory < mh->mh_Upper)
	{
	    ULONG puddleSize = mh->mh_Upper - mh->mh_Lower + 1;

	    /* Found the MemHeader. Free the memory. */
	    Deallocate(mh, memory, memSize);

	    /* Is this MemHeader completely free now? */
	    if(mh->mh_Free == PuddleSize)
	    {
		/* Yes. Remove it from the list. */
		Remove(&mh->mh_Node);

		/* And free it. */
		KrnFreePages((APTR)mh - MEMHEADER_TOTAL, puddleSize + MEMHEADER_TOTAL);
	    }
	    /* All done. */
	    break;
	}
	/* Try next MemHeader */
    }

    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ReleaseSemaphore(&pool->sem);
    }

    AROS_LIBFUNC_EXIT
} /* FreePooled */

