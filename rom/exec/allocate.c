/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Allocate memory from a specific MemHeader.
*/

#define MDEBUG 1

#include <aros/debug.h>
#include <exec/alerts.h>
#include <aros/libcall.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"

/*****************************************************************************

    NAME */

AROS_LH2(APTR, Allocate,

/*  SYNOPSIS */
	AROS_LHA(struct MemHeader *, freeList, A0),
	AROS_LHA(IPTR,               byteSize, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 31, Exec)

/*  FUNCTION
	Allocate memory out of a private region handled by the MemHeader
	structure.

    INPUTS
	freeList - Pointer to the MemHeader structure which holds the memory
	byteSize - Number of bytes you want to get

    RESULT
	A pointer to the number of bytes you wanted or NULL if the memory
	couldn't be allocated

    NOTES
	The memory is aligned to sizeof(struct MemChunk). All requests
	are rounded up to a multiple of that size.

    EXAMPLE
	#define POOLSIZE 4096
	\* Get a MemHeader structure and some private memory *\
	mh=(struct MemHeader *)
	    AllocMem(sizeof(struct MemHeader)+POOLSIZE,MEMF_ANY);
	if(mh!=NULL)
	{
	    \* Build a private pool *\
	    mh->mh_First=(struct MemChunk *)(mh+1);
	    mh->mh_First->mc_Next=NULL;
	    mh->mh_First->mc_Bytes=POOLSIZE;
	    mh->mh_Free=POOLSIZE;
	    {
		\* Use the pool *\
		UBYTE *mem1,*mem2;
		mem1=Allocate(mh,1000);
		mem2=Allocate(mh,2000);
		\* Do something with memory... *\
	    }
	    \* Free everything at once *\
	    FreeMem(mh,sizeof(struct MemHeader)+POOLSIZE);
	}

    BUGS
	Does not work with managed memory blocks because of backwards
	compatibility issues

    SEE ALSO
	Deallocate()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TraceLocation tp = CURRENT_LOCATION("Allocate");
    APTR res;

    D(bug("[exec] Allocate(0x%p, %u)\n", freeList, byteSize));
    ASSERT_VALID_PTR(freeList);

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* Is there enough free memory in the list? */
    if(freeList->mh_Free<byteSize)
	return NULL;

    res = stdAlloc(freeList, byteSize, 0, &tp, SysBase);

    if ((PrivExecBase(SysBase)->IntFlags & EXECF_MungWall) && res) {
	MUNGE_BLOCK(res, MEMFILL_ALLOC, byteSize);
    }

    return res;

    AROS_LIBFUNC_EXIT
} /* Allocate() */
