/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free memory allocated by Allocate().
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <aros/libcall.h>
#include "memory.h"
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH3(void, Deallocate,

/*  SYNOPSIS */
	AROS_LHA(struct MemHeader *, freeList,    A0),
	AROS_LHA(APTR,               memoryBlock, A1),
	AROS_LHA(ULONG,              byteSize,    D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 32, Exec)

/*  FUNCTION
	Free block of memory associated with a given MemHandler structure.

    INPUTS
	freeList    - Pointer to the MemHeader structure
	memoryBlock - Pointer to the memory to be freed
	byteSize    - Size of the block

    RESULT

    NOTES
	The start and end borders of the block are aligned to
	a multiple of sizeof(struct MemChunk) and to include the block.

    EXAMPLE

    BUGS

    SEE ALSO
	Allocate()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemChunk *p1, *p2, *p3;
    UBYTE *p4;

    /* If there is no memory free nothing */
    if(!byteSize)
	return;

    /* Align size to the requirements */
    byteSize+=(IPTR)memoryBlock&(MEMCHUNK_TOTAL-1);
    byteSize=(byteSize+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1);

    /* Align the block as well */
    memoryBlock=(APTR)((IPTR)memoryBlock&~(MEMCHUNK_TOTAL-1));

    /*
	The free memory list is only single linked, i.e. to insert
	elements into the list I need the node as well as it's
	predessor. For the first element I can use freeList->mh_First
	instead of a real predessor.
    */
    p1=(struct MemChunk *)&freeList->mh_First;
    p2=freeList->mh_First;

    /* Start and end(+1) of the block */
    p3=(struct MemChunk *)memoryBlock;
    p4=(UBYTE *)p3+byteSize;

    /* No chunk in list? Just insert the current one and return. */
    if(p2==NULL)
    {
	p3->mc_Bytes=byteSize;
	p3->mc_Next=NULL;
	p1->mc_Next=p3;
	freeList->mh_Free+=byteSize;
	return;
    }

    /* Follow the list to find a place where to insert our memory. */
    do
    {
#if !defined(NO_CONSISTENCY_CHECKS)
	/*
	    Do some constistency checks:
	    1. All MemChunks must be aligned to MEMCHUNK_TOTAL.
	    2. The end (+1) of the current MemChunk
	       must be lower than the start of the next one.
	*/
	if(  ((IPTR)p2|p2->mc_Bytes)&(MEMCHUNK_TOTAL-1)
	    ||(  (UBYTE *)p2+p2->mc_Bytes>=(UBYTE *)p2->mc_Next
		&&p2->mc_Next!=NULL))
	    Alert(AN_MemCorrupt|AT_DeadEnd);
#endif
	/* Found a block with a higher address? */
	if(p2>=p3)
	{
#if !defined(NO_CONSISTENCY_CHECKS)
	    /*
		If the memory to be freed overlaps with the current
		block something must be wrong.
	    */
	    if(p4>(UBYTE *)p2)
	    {
		Alert(AN_FreeTwice);
		return;
	    }
#endif
	    /* End the loop with p2 non-zero */
	    break;
	}
	/* goto next block */
	p1=p2;
	p2=p2->mc_Next;

    /* If the loop ends with p2 zero add it at the end. */
    }while(p2!=NULL);

    /* If there was a previous block merge with it. */
    if(p1!=(struct MemChunk *)&freeList->mh_First)
    {
#if !defined(NO_CONSISTENCY_CHECKS)
	/* Check if they overlap. */
	if((UBYTE *)p1+p1->mc_Bytes>(UBYTE *)p3)
	{
	    Alert(AN_FreeTwice);
	    return;
	}
#endif
	/* Merge if possible */
	if((UBYTE *)p1+p1->mc_Bytes==(UBYTE *)p3)
	    p3=p1;
	else
	    /* Not possible to merge */
	    p1->mc_Next=p3;
    }else
	/*
	    There was no previous block. Just insert the memory at
	    the start of the list.
	*/
	p1->mc_Next=p3;

    /* Try to merge with next block (if there is one ;-) ). */
    if(p4==(UBYTE *)p2&&p2!=NULL)
    {
	/*
	   Overlap checking already done. Doing it here after
	   the list potentially changed would be a bad idea.
	*/
	p4+=p2->mc_Bytes;
	p2=p2->mc_Next;
    }
    /* relink the list and return. */
    p3->mc_Next=p2;
    p3->mc_Bytes=p4-(UBYTE *)p3;
    freeList->mh_Free+=byteSize;
    return;
    AROS_LIBFUNC_EXIT
} /* Deallocate */
