/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate some memory
    Lang: english
*/
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/rt.h>
#include <aros/macros.h>
#include <aros/config.h>
#include <aros/arossupportbase.h>
#include "memory.h"
#include <exec/memory.h>
#include <proto/exec.h>

#include <string.h>

#include "exec_debug.h"
#ifndef DEBUG_AllocMem
#   define DEBUG_AllocMem 0
#endif
#undef DEBUG
#if DEBUG_AllocMem
#   define DEBUG 1
#endif
#define MDEBUG 1
#   include <aros/debug.h>

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, AllocMem,

/*  SYNOPSIS */
	AROS_LHA(ULONG, byteSize,     D0),
	AROS_LHA(ULONG, requirements, D1),

/* LOCATION */
	struct ExecBase *, SysBase, 33, Exec)

/*  FUNCTION
	Allocate some memory from the sytem memory pool with the given
	requirements.

    INPUTS
	byteSize     - Number of bytes you want to get
	requirements - Type of memory

    RESULT
	A pointer to the number of bytes you wanted or NULL if the memory
	couldn't be allocated

    NOTES
	The memory is aligned to sizeof(struct MemChunk). All requests
	are rounded up to a multiple of that size.

    EXAMPLE
	mytask=(struct Task *)AllocMem(sizeof(struct Task),MEMF_PUBLIC|MEMF_CLEAR);

    BUGS

    SEE ALSO
	FreeMem()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Interrupt *lmh;
    APTR res = NULL;
#if ENABLE_RT || AROS_MUNGWALL_DEBUG
    ULONG origSize = byteSize;
#endif

    D(bug("Call AllocMem (%d, %08lx)\n", byteSize, requirements));

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	goto end;

#if AROS_MUNGWALL_DEBUG
    /* Make room for safety walls around allocated block and an some more extra space
       for other interesting things, actually --> the size.

       This all will look like this:

       [MEMCHUNK_FOR_EXTRA_STUFF][BEFORE-MUNGWALL][<alloced-memory-for-user>][AFTER_MUNGWALL]

       The first ULONG in MEMCHUNK_FOR_EXTRA_STUFF is used to save the original alloc
       size (byteSize) param. So it is possible in FreeMem to check, if freemem size
       matches allocmem size or not.
    */

    byteSize += MUNGWALL_SIZE * 2 + MUNGWALLHEADER_SIZE;
#endif /* AROS_MUNGWALL_DEBUG */

    /* First round byteSize to a multiple of MEMCHUNK_TOTAL */
    byteSize = AROS_ROUNDUP2(byteSize, MEMCHUNK_TOTAL);

    /* Protect memory list against other tasks */
    Forbid();

    /* Loop over low memory handlers */
    lmh=(struct Interrupt *)SysBase->ex_MemHandlers.mlh_Head;
    for(;;)
    {
	struct MemHeader *mh;

	/* Loop over MemHeader structures */
	mh=(struct MemHeader *)SysBase->MemList.lh_Head;
	while(mh->mh_Node.ln_Succ!=NULL)
	{
	    struct MemChunk *p1,*p2;

	    /*
		Check for the right requirements and enough free memory.
		The requirements are OK if there's no bit in the
		'attributes' that isn't set in the 'mh->mh_Attributes'.
		MEMF_CLEAR, MEMF_REVERSE and MEMF_NO_EXPUNGE are treated
		as if they were always set in the memheader.
	    */
	    if(!(requirements&~(MEMF_CLEAR|MEMF_REVERSE|
				MEMF_NO_EXPUNGE|mh->mh_Attributes))
	       &&mh->mh_Free>=byteSize)
	    {
		struct MemChunk *mc=NULL;

		/*
		    The free memory list is only single linked, i.e. to remove
		    elements from the list I need node's predessor. For the
		    first element I can use mh->mh_First instead of a real predessor.
		*/
		p1=(struct MemChunk *)&mh->mh_First;
		p2=p1->mc_Next;

		/* Is there anything in the list? */
		if(p2!=NULL)
		{
		    /* Then follow it */
		    for(;;)
		    {
#if !defined(NO_CONSISTENCY_CHECKS)
			/* Consistency check: Check alignment restrictions */
			if( ((IPTR)p2|(ULONG)p2->mc_Bytes)
			   & (MEMCHUNK_TOTAL-1) )
			    Alert(AN_MemCorrupt|AT_DeadEnd);
#endif
			/* Check if the current block is large enough */
			if(p2->mc_Bytes>=byteSize)
			{
			    /* It is. */
			    mc=p1;
			    /* Use this one if MEMF_REVERSE is not set.*/
			    if(!(requirements&MEMF_REVERSE))
				break;
			    /* Else continue - there may be more to come. */
			}

			/* Go to next block */
			p1=p2;
			p2=p1->mc_Next;

			/* Check if this was the end */
			if(p2==NULL)
			    break;
#if !defined(NO_CONSISTENCY_CHECKS)
			/*
			    Consistency check:
			    If the end of the last block+1 is bigger or equal to
			    the start of the current block something must be wrong.
			*/
			if((UBYTE *)p2<=(UBYTE *)p1+p1->mc_Bytes)
			    Alert(AN_MemCorrupt|AT_DeadEnd);
#endif
		    }
		    /* Something found? */
		    if(mc!=NULL)
		    {
			/*
			    Remember: if MEMF_REVERSE is set
			    p1 and p2 are now invalid.
			*/
			p1=mc;
			p2=p1->mc_Next;

			/* Remove the block from the list and return it. */
			if(p2->mc_Bytes==byteSize)
			{
			    /* Fits exactly. Just relink the list. */
			    p1->mc_Next=p2->mc_Next;
			    mc=p2;
			}else
			{
			    if(requirements&MEMF_REVERSE)
			    {
				/* Return the last bytes. */
				p1->mc_Next=p2;
				mc=(struct MemChunk *)((UBYTE *)p2+p2->mc_Bytes-byteSize);
			    }else
			    {
				/* Return the first bytes. */
				p1->mc_Next=(struct MemChunk *)((UBYTE *)p2+byteSize);
				mc=p2;
			    }
			    p1=p1->mc_Next;
			    p1->mc_Next=p2->mc_Next;
			    p1->mc_Bytes=p2->mc_Bytes-byteSize;
			}
			mh->mh_Free-=byteSize;

			/* No need to forbid dispatching any longer. */
			Permit();
			if(requirements&MEMF_CLEAR)
			{
			    /* Clear memory. */
			    ULONG cnt,*p;

			    p=(ULONG *)mc;
			    cnt=byteSize/sizeof(ULONG);

			    while(cnt--)
				*p++=0;
			}
			res=mc;
			goto end;
		    }
		}
	    }
	    /* Go to next memory header */
	    mh=(struct MemHeader *)mh->mh_Node.ln_Succ;
	}

	/* Is it forbidden to call low-memory handlers? */
	if(requirements&MEMF_NO_EXPUNGE)
	{
	    Permit();
	    goto end;
	}

	/* All memory headers done. Check low memory handlers. */
	{
	    ULONG lmhr;
	    struct MemHandlerData lmhd={ byteSize,requirements,0 };

	    do
	    {
		/* Is there another one? */
		if(lmh->is_Node.ln_Succ==NULL)
		{
		    /* No. return 'Not enough memory'. */
		    Permit();
		    goto end;
		}

		/* Yes. Execute it. */
		lmhr = AROS_UFC3 (LONG, lmh->is_Code,
		    AROS_UFCA(struct MemHandlerData *,&lmhd,A0),
		    AROS_UFCA(APTR,lmh->is_Data,A1),
		    AROS_UFCA(struct ExecBase *,SysBase,A6)
		);

		/* Check returncode. */
		if(lmhr==MEM_TRY_AGAIN)
		{
		    /* MemHandler said he did something. Try again. */
		    /* Is there any program that depends on this flag??? */
		    lmhd.memh_Flags|=MEMHF_RECYCLE;
		    break;
		}
		/* Nothing more to expect from this handler. */
		lmh=(struct Interrupt *)lmh->is_Node.ln_Succ;
		lmhd.memh_Flags&=~MEMHF_RECYCLE;

	    /* If this handler did nothing at all there's no need
	     * to try the allocation. Try the next handler immediately.
	     */
	    } while(lmhr==MEM_DID_NOTHING);
	}
    }

end:

#if ENABLE_RT
    RT_Add (RTT_MEMORY, res, origSize);
#endif

#if AROS_MUNGWALL_DEBUG
    if (res)
    {
    	struct MungwallHeader *header;
	struct List 	      *allocmemlist;
	
        /* Save orig byteSize before wall (there is one room of MUNGWALLHEADER_SIZE
	   bytes before wall for such stuff (see above).
	*/
	
	header = (struct MungwallHeader *)res;
	
	header->mwh_magicid = MUNGWALL_HEADER_ID;
	header->mwh_allocsize = origSize;

	/* Check whether list does exist. AllocMem() might have been
	   called before PrepareAROSSupportBase() which is responsible for
	   initialization of AllocMemList */
	   
	if (SysBase->DebugAROSBase)
	{	
    	    allocmemlist = (struct List *)&((struct AROSSupportBase *)SysBase->DebugAROSBase)->AllocMemList;
	    Forbid();
    	    AddHead(allocmemlist, (struct Node *)&header->mwh_node);
	    Permit();
	}
	else
	{
	    header->mwh_node.mln_Pred = (struct MinNode *)0x44332211;
	    header->mwh_node.mln_Succ = (struct MinNode *)0xCCBBAA99;
	}
	
	/* Skip to the start of the pre-wall */
        res += MUNGWALLHEADER_SIZE;

	/* Initialize pre-wall */
	BUILD_WALL(res, 0xDB, MUNGWALL_SIZE);

	/* move over the block between the walls */
	res += MUNGWALL_SIZE;

	/* Fill the block with weird stuff to exploit bugs in applications */
	if (!(requirements & MEMF_CLEAR))
	    MUNGE_BLOCK(res, MEMFILL_ALLOC, byteSize - MUNGWALL_SIZE * 2 - MEMCHUNK_TOTAL);

	/* Initialize post-wall */
	BUILD_WALL(res + origSize, 0xDB, MUNGWALL_SIZE + AROS_ROUNDUP2(origSize, MEMCHUNK_TOTAL) - origSize);
    }
#endif /* AROS_MUNGWALL_DEBUG */

    ReturnPtr ("AllocMem", APTR, res);
    
    AROS_LIBFUNC_EXIT
    
} /* AllocMem */

