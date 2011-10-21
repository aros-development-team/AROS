/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a memory pool.
    Lang: english
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <clib/alib_protos.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"
#include "mungwall.h"

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
		       smaller than or equal to the puddleSize.

    RESULT
	A handle for the memory pool or NULL if the pool couldn't
	be created

    NOTES
	Since exec.library v41.12 implementation of pools has been rewritten
	to make use of memory protection capabilities. threshSize parameter
	is effectively ignored and is present only for backwards compatibility.

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

    struct TraceLocation tp = CURRENT_LOCATION("CreatePool");
    struct MemHeader *firstPuddle = NULL;
    ULONG align = PrivExecBase(SysBase)->PageSize - 1;

    D(bug("[exec] CreatePool(0x%08X, %u, %u)\n", requirements, puddleSize, threshSize));
    
    /*
     * puddleSize needs to include MEMHEADER_TOTAL and one pointer.
     * This is because our puddles must be able to accomodate an allocation
     * of own size. Allocations of larger size will always use enlarged puddles.
     * Pointer is used for pointing back to the MemHeader from which the block
     * was allocated, in AllocVec()-alike manner. This way we get rid of slow lookup
     * in FreePooled().
     */
    puddleSize += MEMHEADER_TOTAL + sizeof(struct MemHeader *);

    /* If mungwall is enabled, count also size of walls, at least for one allocation */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
        puddleSize += MUNGWALL_TOTAL_SIZE;

    /* Then round puddleSize up to be a multiple of page size. */
    puddleSize = (puddleSize + align) & ~align;
    D(bug("[CreatePool] Aligned puddle size: %u (0x%08X)\n", puddleSize, puddleSize));

    /* Allocate the first puddle. It will contain pool header. */
    firstPuddle = AllocMemHeader(puddleSize, requirements, &tp, SysBase);
    D(bug("[CreatePool] Initial puddle 0x%p\n", firstPuddle));

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
	D(bug("[CreatePool] Pool header 0x%p (size %u)\n", pool, poolstruct_size));

	/* Initialize pool header */
	NEWLIST((struct List *)&pool->pool.PuddleList);
	pool->pool.Requirements = requirements;
	pool->pool.PuddleSize   = puddleSize;

	if (requirements & MEMF_SEM_PROTECTED)
	{
	    InitSemaphore(&pool->sem);
	}

	/*
	 * Add the puddle to the list (yes, contained in itself).
	 * This is the first puddle so it's safe to use AddTail() here.
	 * Note that we use ln_Name of our MemHeader to point back to
	 * our pool.
	 */
	firstPuddle->mh_Node.ln_Name = (STRPTR)pool;
	AddTail((struct List *)&pool->pool.PuddleList, &firstPuddle->mh_Node);
    }

    return firstPuddle;

    AROS_LIBFUNC_EXIT
} /* CreatePool */
