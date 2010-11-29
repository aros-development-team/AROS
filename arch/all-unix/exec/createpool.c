/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a memory pool.
    Lang: english
*/

#include <aros/libcall.h>
#include <clib/alib_protos.h>

#include "exec_intern.h"
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
		       smaller or equal than the puddleSize.

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

    struct MemHeader *firstPuddle = NULL;

    /* puddleSize must not be smaller than threshSize */
    if(puddleSize >= threshSize)
    {
    	ULONG align = PrivExecBase(SysBase)->PageSize - 1;
    	/*
    	 * In future we are going to have MEMF_EXECUTABLE, and MAP_Executable will depend on it
    	 */
    	KRN_MapAttr prot = MAP_Readable|MAP_Writable|MAP_Executable;

	/* puddleSize needs to include MEMHEADER_TOTAL */
	puddleSize += MEMHEADER_TOTAL;
	/* Then round puddleSize up to be a multiple of page size. */
	puddleSize = (puddleSize + align) & ~align;

	/*
	    Allocate memory for the Pool structure using the same requirements
	    as for the whole pool (to make it shareable, residentable or
	    whatever is needed).
	*/

	/* Allocate the first puddle. It will contain pool header. */
	firstPuddle = AllocMemHeader(puddleSize, requirements, prot, SysBase);
	if (firstPuddle)
	{
	    ULONG poolstruct_size = (requirements & MEMF_SEM_PROTECTED) ? sizeof(struct ProtectedPool) :
	    	    	    	    	    	    	        	  sizeof(struct Pool);
	    struct ProtectedPool *pool;

	    /*
	     * Allocate pool header inside the puddle.
	     * It is the first allocation in this puddle, so in future we can always find
	     * header's address as poolbase + MEMHEADER_TOTAL.
	     */
	    pool = Allocate(firstPuddle, poolstruct_size);

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

	    /* Add the puddle to the list (yes, contained in itself) */
	    AddTail((struct List *)&pool->PuddleList, (struct Node *)firstPuddle);
	}
    } /* if(puddleSize >= threshSize) */

    return firstPuddle;

    AROS_LIBFUNC_EXIT
    
} /* CreatePool */
