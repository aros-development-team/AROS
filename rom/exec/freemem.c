/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free memory allocated by AllocMem()
    Lang: english
*/
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/config.h>
#include <aros/macros.h>
#include "memory.h"
#include <aros/rt.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_debug.h"
#ifndef DEBUG_FreeMem
#   define DEBUG_FreeMem 0
#endif
#undef DEBUG
#if DEBUG_FreeMem
#   define DEBUG 1
#endif
#define MDEBUG 1

#include <aros/debug.h>

#include <stdlib.h>

/*****************************************************************************

    NAME */

	AROS_LH2(void, FreeMem,

/*  SYNOPSIS */
	AROS_LHA(APTR,  memoryBlock, A1),
	AROS_LHA(ULONG, byteSize,    D0),

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemHeader *mh;
    struct MemChunk *p1, *p2, *p3;
    UBYTE *p4;
#if AROS_MUNGWALL_DEBUG
    ULONG origsize = byteSize
#endif

    D(bug("Call FreeMem (%08lx, %ld)\n", memoryBlock, byteSize));

    /* If there is no memory free nothing */
    if(!byteSize)
	ReturnVoid ("FreeMem");

    RT_Free (RTT_MEMORY, memoryBlock, byteSize);

    /* The following two lines are necessary because of AllocAbs(),
       were memoryBlock might not be aligned to a multiple of
       MEMCHUNK_TOTAL!!!!  */
    
    /* Align size to the requirements (needed because of AllocAbs) */
    byteSize+=(IPTR)memoryBlock&(MEMCHUNK_TOTAL-1);

    /* Align the block as well (needed because of AllocAbs) */
    memoryBlock=(APTR)AROS_ROUNDDOWN2((IPTR)memoryBlock,MEMCHUNK_TOTAL);

#if AROS_MUNGWALL_DEBUG
	/* Add the size of mung walls and mungwall header */
	memoryBlock -= MUNGWALL_SIZE + MUNGWALLHEADER_SIZE;
	byteSize += MUNGWALL_SIZE * 2 + MUNGWALLHEADER_SIZE;
#endif

    byteSize=AROS_ROUNDUP2(byteSize,MEMCHUNK_TOTAL);

#if AROS_MUNGWALL_DEBUG
    {
    	struct MungwallHeader *header;
	
	header = (struct MungwallHeader *)memoryBlock;
	
	if (header->mwh_magicid != MUNGWALL_HEADER_ID)
	{
	    struct Task *__t = FindTask(NULL);	\
	    kprintf("\x07MUNGWALL_HEADER_ID mismatch (%s) mem = %x"
		    "allocsize = %d  freesize = %d   Task: 0x%x, Name: %s\n", \
		    __FUNCTION__,
		    memoryBlock + MUNGWALL_SIZE + MUNGWALLHEADER_SIZE,
		    *(ULONG *)memoryBlock,
		    origsize,
		    __t,
		    __t->tc_Node.ln_Name);\
	}
	
	if (header->mwh_allocsize != origsize)
	{
	    struct Task *__t = FindTask(NULL);	\
	    kprintf("\x07FreeMem size mismatches AllocMem size (%s) mem = %x"
		    "allocsize = %d  freesize = %d   Task: 0x%x, Name: %s\n", \
		    __FUNCTION__,
		    memoryBlock + MUNGWALL_SIZE + MUNGWALLHEADER_SIZE,
		    *(ULONG *)memoryBlock,
		    origsize,
		    __t,
		    __t->tc_Node.ln_Name);\
	}
	
	CHECK_WALL((UBYTE *)memoryBlock + MUNGWALLHEADER_SIZE, 0xDB, MUNGWALL_SIZE);
	CHECK_WALL((UBYTE *)memoryBlock + MUNGWALLHEADER_SIZE + MUNGWALL_SIZE + origsize, 0xDB,
		MUNGWALL_SIZE + AROS_ROUNDUP2(origsize, MEMCHUNK_TOTAL) - origsize);

    	/* Remove from AROSSupportBase->AllocMemList */
	
	if ((header->mwh_node.mln_Pred != (struct MinNode *)0x44332211) ||
	    (header->mwh_node.mln_Succ != (struct MinNode *)0xCCBBAA99))
	{
	    /* Reason for above checks: see allocmem.c */
	    Forbid();
    	    Remove((struct Node *)&header->mwh_node);
	    Permit();
    	}
	
	/* Fill block with weird stuff to esploit bugs in applications
	 *
	 * DOH! There's some _BAD_ code around that assumes memory can still be
	 * accessed after freeing by just preventing task switching. In AROS,
	 * RemTask(NULL) suffers of this problem because DOS processes are
	 * created with their TCB placed in the tc_MemEntry list. The workaround
	 * is to avoid munging when FreeMem() is called with task switching disabled.
	 */
	/* DOH! it doesn't work even this way. What's wrong???
	 * 
	 * if ((SysBase->TDNestCnt < 0) && (SysBase->IDNestCnt < 0))
	 *	MUNGE_BLOCK(memoryBlock, MEMFILL_FREE, byteSize);
	 */
	
    }
#endif

    /* Start and end(+1) of the block */
    p3=(struct MemChunk *)memoryBlock;
    p4=(UBYTE *)p3+byteSize;

    /* Protect the memory list from access by other tasks. */
    Forbid();

    ForeachNode(&SysBase->MemList, mh)
    {
	/* Test if the memory belongs to this MemHeader. */
	if (mh->mh_Lower > memoryBlock || mh->mh_Upper <= memoryBlock)
	    continue;
	    
	if (mh->mh_Attributes & MEMF_MANAGED)
	{
	    struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;
	    
	    if (mhe->mhe_Free)
	        mhe->mhe_Free(mhe, memoryBlock, byteSize);
	   
  	    Permit();
	        ReturnVoid ("FreeMem");
        }

	    
	#if !defined(NO_CONSISTENCY_CHECKS)
	/* Test if it really fits into this MemHeader. */
	if ((APTR)p4 > mh->mh_Upper)
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
	if (p2 == NULL)
	{
	    p3->mc_Bytes = byteSize;
	    p3->mc_Next  = NULL;
	    p1->mc_Next  = p3;
	    mh->mh_Free += byteSize;
	    
	    Permit();
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
                    Alert(AN_FreeTwice|AT_DeadEnd);
                #endif
                /* End the loop with p2 non-zero */
                break;
            }
            /* goto next block */
            p1=p2;
            p2=p2->mc_Next;
    
            /* If the loop ends with p2 zero add it at the end. */
	} while (p2 != NULL);

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
	}
        else
        {
            /*
                There was no previous block. Just insert the memory at
                the start of the list.
            */
            p1->mc_Next=p3;
        }

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
	p3->mc_Next   = p2;
	p3->mc_Bytes  = p4-(UBYTE *)p3;
	mh->mh_Free  += byteSize;
	Permit();
	ReturnVoid ("FreeMem");
    }

#if !defined(NO_CONSISTENCY_CHECKS)
    /* Some memory that didn't fit into any MemHeader? */
    Alert(AN_MemCorrupt|AT_DeadEnd);
#else
    Permit();
#endif

    ReturnVoid ("FreeMem");
    
    AROS_LIBFUNC_EXIT
} /* FreeMem */

