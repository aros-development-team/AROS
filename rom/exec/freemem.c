/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1996/08/16 14:05:12  digulla
    Added debug output

    Revision 1.6  1996/08/15 14:42:15  digulla
    Added comment

    Revision 1.5  1996/08/15 13:19:02  digulla
    First attempt to purge memory after free to make code crash which accesses
    memory after a free but just that happens in RemTask().

    Revision 1.4  1996/08/13 13:56:02  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:11  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/alerts.h>
#include <exec/execbase.h>
#include "machine.h"
#include "memory.h"

#include "exec_debug.h"
#ifndef DEBUG_FreeMem
#   define DEBUG_FreeMem 0
#endif
#if DEBUG_FreeMem
#   undef DEBUG
#   define DEBUG 1
#endif
#include <aros/debug.h>

#define NASTY_FREEMEM	0	/* Delete contents of free'd memory ? */

#if NASTY_FREEMEM
void PurgeChunk (ULONG *, ULONG);
#endif

/*****************************************************************************

    NAME */
	#include <exec/memory.h>
	#include <clib/exec_protos.h>

	__AROS_LH2(void, FreeMem,

/*  SYNOPSIS */
	__AROS_LHA(APTR,  memoryBlock, A1),
	__AROS_LHA(ULONG, byteSize,    D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 35, Exec)

/*  FUNCTION
	Give a block of memory back to the system pool.

    INPUTS
	memoryBlock - Pointer to the memory to be freed
	byteSize    - Size of the block

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocMem()

    INTERNALS

    HISTORY
	8-10-95    created by m. fleischer
       16-10-95    increased portability

******************************************************************************/
{
    __AROS_FUNC_INIT

    struct MemHeader *mh;
    struct MemChunk *p1, *p2, *p3;
    UBYTE *p4;

    D(bug("Call FreeMem (%08lx, %ld)\n", memoryBlock, byteSize));

    /* If there is no memory free nothing */
    if(!byteSize)
	ReturnVoid ("FreeMem");

    /* Align size to the requirements */
    byteSize+=(ULONG)memoryBlock&(MEMCHUNK_TOTAL-1);
    byteSize=(byteSize+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1);

    /* Align the block as well */
    memoryBlock=(APTR)((ULONG)memoryBlock&~(MEMCHUNK_TOTAL-1));

    /* Start and end(+1) of the block */
    p3=(struct MemChunk *)memoryBlock;
    p4=(UBYTE *)p3+byteSize;

    /* Protect the memory list from access by other tasks. */
    Forbid();

    /* Loop over MemHeader structures */
    mh=(struct MemHeader *)SysBase->MemList.lh_Head;
    while(mh->mh_Node.ln_Succ)
    {
	/* Test if the memory belongs to this MemHeader. */
	if(mh->mh_Lower<=memoryBlock&&mh->mh_Upper>memoryBlock)
	{
#if !defined(NO_CONSISTENCY_CHECKS)
	    /* Test if it really fits into this MemHeader. */
	    if((APTR)p4>mh->mh_Upper)
		/* Something is completely wrong. */
		Alert(AN_MemCorrupt|AT_DeadEnd);
#endif
	    /*
		The free memory list is only single linked, i.e. to insert
		elements into the list I need the node as well as it's
		predessor. For the first element I can use freeList->mh_First
		instead of a real predessor.
	    */
	    p1=(struct MemChunk *)&mh->mh_First;
	    p2=p1->mc_Next;

	    /* No chunk in list? Just insert the current one and return. */
	    if(p2==NULL)
	    {
#if NASTY_FREEMEM
		PurgeChunk ((ULONG *)p3, byteSize);
#endif
		p3->mc_Bytes=byteSize;
		p3->mc_Next=NULL;
		p1->mc_Next=p3;
		mh->mh_Free+=byteSize;
		Permit ();
		ReturnVoid ("FreeMem");
	    }

	    /* Follow the list to find a place where to insert our memory. */
	    do
	    {
#if !defined(NO_CONSISTENCY_CHECKS)
		/*
		    Do some constistency checks:
		    1. All MemChunks must be aligned to
		       MEMCHUNK_TOTAL.
		    2. The end (+1) of the current MemChunk
		       must be lower than the start of the next one.
		*/
		if(  ((ULONG)p2|p2->mc_Bytes)&(MEMCHUNK_TOTAL-1)
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
			Alert(AN_FreeTwice|AT_DeadEnd);
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
	    if(p1!=(struct MemChunk *)&mh->mh_First)
	    {
#if !defined(NO_CONSISTENCY_CHECKS)
		/* Check if they overlap. */
		if((UBYTE *)p1+p1->mc_Bytes>(UBYTE *)p3)
		    Alert(AN_FreeTwice|AT_DeadEnd);
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
#if NASTY_FREEMEM
	    PurgeChunk ((ULONG *)p3, p4-(UBYTE *)p3);
#endif
	    p3->mc_Next=p2;
	    p3->mc_Bytes=p4-(UBYTE *)p3;
	    mh->mh_Free+=byteSize;
	    Permit();
	    ReturnVoid ("FreeMem");
	}
	mh=(struct MemHeader *)mh->mh_Node.ln_Succ;
    }

#if !defined(NO_CONSISTENCY_CHECKS)
    /* Some memory that didn't fit into any MemHeader? */
    Alert(AN_MemCorrupt|AT_DeadEnd);
#else
    Permit();
#endif

    ReturnVoid ("FreeMem");
    __AROS_FUNC_EXIT
} /* FreeMem */


/* Don't use #if on this one since it may be used by some other routine, too.
    It's not static by design. */
void PurgeChunk (ULONG * ptr, ULONG size)
{
    while (size >= sizeof (ULONG))
    {
#if SIZEOFULONG > 4
	*ptr ++ = 0xDEAFBEEFDEADBEEFL;
#else
	*ptr ++ = 0xDEAFBEEFL;
#endif
	size -= sizeof (ULONG);
    }
}
