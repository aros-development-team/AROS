/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate memory in a pool.
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include "memory.h"

#include "exec_debug.h"
#ifndef DEBUG_AllocPooled
#   define DEBUG_AllocPooled 0
#endif
#undef DEBUG
#if DEBUG_AllocPooled
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <exec/memory.h>
#include <proto/exec.h>

	AROS_LH2(APTR, AllocPooled,

/*  SYNOPSIS */
	AROS_LHA(APTR,  poolHeader, A0),
	AROS_LHA(ULONG, memSize,    D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 118, Exec)

/*  FUNCTION
	Allocate memory out of a private memory pool.

    INPUTS
	poolHeader - Handle of the memory pool
	memSize    - Number of bytes you want to get

    RESULT
	A pointer to the number of bytes you wanted or NULL if the memory
	couldn't be allocated

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePool(), DeletePool(), FreePooled()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ProtectedPool *pool = (struct ProtectedPool *)poolHeader;
    APTR    	    	 ret = NULL;

    D(bug("AllocPooled $%lx memsize $%lx by \"%s\"\n", poolHeader, memSize, SysBase->ThisTask->tc_Node.ln_Name));

    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ObtainSemaphore(&pool->sem);
    }

    /* If the memSize is bigger than the ThreshSize allocate seperately. */
    if(memSize > pool->pool.ThreshSize)
    {
	struct Block *bl;
	ULONG 	     size;

	/* Get enough memory for the memory block including the header. */
	size = memSize + BLOCK_TOTAL;
	bl = (struct Block *)AllocMem(size, pool->pool.Requirements & ~MEMF_SEM_PROTECTED);

	/* No memory left */
	if(bl == NULL)
	    goto done;

	/* Initialize the header */
	bl->Size = size;

	/* Add the block to the BlockList */
	AddHead((struct List *)&pool->pool.BlockList,(struct Node *)&bl->Node);

	/* Set pointer to allocated memory */
	ret = (UBYTE *)bl + BLOCK_TOTAL;
    }
    else
    {
	struct MemHeader *mh;

	/* Follow the list of MemHeaders */
	mh = (struct MemHeader *)pool->pool.PuddleList.mlh_Head;
	for(;;)
	{
	    /* Are there no more MemHeaders? */
	    if(mh->mh_Node.ln_Succ==NULL)
	    {
		/* Get a new one */
		mh = (struct MemHeader *)
		   AllocMem(pool->pool.PuddleSize + MEMHEADER_TOTAL,
		    	    pool->pool.Requirements & ~MEMF_SEM_PROTECTED);

		/* No memory left? */
		if(mh == NULL)
		    goto done;

		/* Initialize new MemHeader */
		mh->mh_First	    	= (struct MemChunk *)((UBYTE *)mh + MEMHEADER_TOTAL);
		mh->mh_First->mc_Next 	= NULL;
		mh->mh_First->mc_Bytes  = pool->pool.PuddleSize;
		mh->mh_Lower 	    	= mh->mh_First;
		mh->mh_Upper 	    	= (UBYTE *)mh->mh_First+pool->pool.PuddleSize;
		mh->mh_Free  	    	= pool->pool.PuddleSize;

		/* And add it to the list */
		AddHead((struct List *)&pool->pool.PuddleList, (struct Node *)&mh->mh_Node);
		/* Fall through to get the memory */
	    }
	    /* Try to get the memory */
	    ret = Allocate(mh, memSize);

	    /* Got it? */
	    if(ret != NULL)
		break;

	    /* No. Try next MemHeader */
	    mh = (struct MemHeader *)mh->mh_Node.ln_Succ;
	}
	/* Allocate does not clear the memory! */
	if(pool->pool.Requirements & MEMF_CLEAR)
	{
	    ULONG *p= (ULONG *)ret;

	    /* Round up (clearing longs is faster than just bytes) */
	    memSize = (memSize + sizeof(ULONG) - 1) / sizeof(ULONG);

	    /* NUL the memory out */
	    while(memSize--)
		*p++=0;
	}
    }

done:
    if (pool->pool.Requirements & MEMF_SEM_PROTECTED)
    {
    	ReleaseSemaphore(&pool->sem);
    }
    
    /* Everything fine */
    return ret;
    
    AROS_LIBFUNC_EXIT
    
} /* AllocPooled */

