/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Allocate memory from a specific MemHeader.
*/

#include "exec_intern.h"
#include "memory.h"
#include <exec/alerts.h>
#include <aros/libcall.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

AROS_LH2(APTR, Allocate,

/*  SYNOPSIS */
	AROS_LHA(struct MemHeader *, freeList, A0),
	AROS_LHA(ULONG,              byteSize, D0),

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

    SEE ALSO
	Deallocate()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct MemChunk *p1, *p2;

    ASSERT_VALID_PTR(freeList);


    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* First round byteSize to a multiple of MEMCHUNK_TOTAL. */
    byteSize=AROS_ROUNDUP2(byteSize,MEMCHUNK_TOTAL);

    /* Is there enough free memory in the list? */
    if(freeList->mh_Free<byteSize)
	return NULL;

    /*
	The free memory list is only single linked, i.e. to remove
	elements from the list I need the node as well as it's
	predessor. For the first element I can use freeList->mh_First
	instead of a real predecessor.
    */
    p1=(struct MemChunk *)&freeList->mh_First;
    p2=p1->mc_Next;

    /* Is the list enpty? */
    if(p2==NULL)
	return NULL;

    /* Follow the list */
    for(;;)
    {
#if !defined(NO_CONSISTENCY_CHECKS)
	/* Consistency check: Check alignment restrictions */
	if( ((IPTR)p2|(ULONG)p2->mc_Bytes) & (MEMCHUNK_TOTAL-1) )
	{
	    if (SysBase != NULL) Alert(AN_MemCorrupt);
	    return NULL;
	}
#endif
	/* Check if current block is large enough */
	if(p2->mc_Bytes>=byteSize)
	{
	    /* It is. Remove it from the list and return it. */
	    if(p2->mc_Bytes==byteSize)
		/* Fits exactly. Just relink the list. */
		p1->mc_Next=p2->mc_Next;
	    else
	    {
		/* Split the current chunk and return the first bytes. */
		p1->mc_Next=(struct MemChunk *)((UBYTE *)p2+byteSize);
		p1=p1->mc_Next;
		p1->mc_Next=p2->mc_Next;
		p1->mc_Bytes=p2->mc_Bytes-byteSize;
	    }
	    /* Adjust free memory count and return */
	    freeList->mh_Free-=byteSize;

	    /* Fill the block with weird stuff to exploit bugs in applications */
	    MUNGE_BLOCK(p2,MEMFILL_ALLOC,byteSize);

	    /* Return allocated block to caller */
	    return p2;
	}

	/* Go to next block */
	p1=p2;
	p2=p1->mc_Next;

	/* Check if this was the end */
	if(p2==NULL)
	    return NULL;
#if !defined(NO_CONSISTENCY_CHECKS)
	/*
	    Consistency check:
	    If the end of the last block+1 is bigger or equal to
	    the start of the current block something must be wrong.
	*/
	if((UBYTE *)p2<=(UBYTE *)p1+p1->mc_Bytes)
	{
	    if (SysBase != NULL) Alert(AN_MemCorrupt);
	    return NULL;
	}
#endif
    }
    AROS_LIBFUNC_EXIT
} /* Allocate() */
