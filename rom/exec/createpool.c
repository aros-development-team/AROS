/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a memory pool.
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <clib/alib_protos.h>
#include "memory.h"

/*****************************************************************************

    NAME */
#include <exec/memory.h>
#include <proto/exec.h>

	AROS_LH3(APTR, CreatePool,

/*  SYNOPSIS */
	AROS_LHA(ULONG, requirements, D0),
	AROS_LHA(ULONG, puddleSize,   D1),
	AROS_LHA(ULONG, threshSize,   D2),

/*  LOCATION */
	struct ExecBase *, SysBase, 116, Exec)

/*  FUNCTION
	Create a private pool for memory allocations.

    INPUTS
	requirements - The type of the memory
	puddleSize   - The number of bytes that the pool expands
		       if it is too small.
	threshSize   - Allocations beyond the threshSize are given
		       directly to the system. threshSize must be
		       smaller than the puddleSize.

    RESULT
	A handle for the memory pool or NULL if the pool couldn't
	be created

    NOTES

    EXAMPLE
	\* Get the handle to a private memory pool *\
	po=CreatePool(MEMF_ANY,16384,8192);
	if(po!=NULL)
	{
	    \* Use the pool *\
	    UBYTE *mem1,*mem2;
	    mem1=AllocPooled(po,1000);
	    mem2=AllocPooled(po,2000);
	    \* Do something with the memory... *\

	    \* Free everything at once *\
	    DeletePool(po);
	}

    BUGS

    SEE ALSO
	DeletePool(), AllocPooled(), FreePooled()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ProtectedPool *pool = NULL;

    /* puddleSize must not be smaller than threshSize */
    if(puddleSize >= threshSize)
    {
    	LONG poolstruct_size;
	
	/* Round puddleSize up to be a multiple of MEMCHUNK_TOTAL. */
	puddleSize = (puddleSize + MEMCHUNK_TOTAL - 1) & ~(MEMCHUNK_TOTAL - 1);

	/*
	    Allocate memory for the Pool structure using the same requirements
	    as for the whole pool (to make it shareable, residentable or
	    whatever is needed).
	*/
	
	poolstruct_size = (requirements & MEMF_SEM_PROTECTED) ? sizeof(struct ProtectedPool) :
	    	    	    	    	    	    	        sizeof(struct Pool);
							       
	pool=(struct ProtectedPool *)AllocMem(poolstruct_size, requirements & ~MEMF_SEM_PROTECTED);
	if(pool != NULL)
	{
	    /* Clear the lists */
	    NEWLIST((struct List *)&pool->pool.PuddleList);
	    NEWLIST((struct List *)&pool->pool.BlockList );

	    /* Set the rest */
	    pool->pool.Requirements = requirements;
	    pool->pool.PuddleSize   = puddleSize;
	    pool->pool.ThreshSize   = threshSize;
	    
	    if (requirements & MEMF_SEM_PROTECTED)
	    {
	    	InitSemaphore(&pool->sem);
	    }
	}
	
    } /* if(puddleSize >= threshSize) */
    
    return pool;
    
    AROS_LIBFUNC_EXIT
    
} /* CreatePool */
