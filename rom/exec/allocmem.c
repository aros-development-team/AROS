/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:04  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang:
*/
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <machine.h>
#include "memory.h"

/*****************************************************************************

    NAME */
	#include <exec/memory.h>
	#include <clib/exec_protos.h>

__AROS_LH2(APTR, AllocMem,

/*  SYNOPSIS */
	__AROS_LA(ULONG, byteSize,     D0),
	__AROS_LA(ULONG, requirements, D1),

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

    HISTORY
	8-10-95    created by m. fleischer
       16-10-95    increased portability

******************************************************************************/
{
    __AROS_FUNC_INIT
    struct Interrupt *lmh;
    struct MemHandlerData lmhd={ byteSize,requirements,0 };

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* First round byteSize to a multiple of MEMCHUNK_TOTAL. */
    byteSize=(byteSize+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1);

    /* Protect memory list against other tasks */
    Forbid();

    /* Loop over low memory handlers */
    lmh=(struct Interrupt *)SysBase->ex_MemHandlers.mlh_Head;
    for(;;)
    {
	struct MemHeader *mh;
	ULONG lmhr;

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
			if( ((ULONG)p2|(ULONG)p2->mc_Bytes)
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
				mc=(struct MemChunk *)((UBYTE *)p2+byteSize);
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
			return mc;
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
	    return NULL;
	}

	/* All memory headers done. Check low memory handlers. */
	do
	{
	    /* Is there another one? */
	    if(lmh->is_Node.ln_Succ==NULL)
	    {
		/* No. return 'Not enough memory'. */
		Permit();
		return NULL;
	    }
	    /* Yes. Execute it. */
	    lmhr=__AROS_ABS_CALL3(LONG,lmh->is_Code,&lmhd,A0,lmh->is_Data,A1,SysBase,A6);

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

	/*
	    If this handler did nothing at all there's no need
	    to try the allocation. Try the next handler immediately.
	*/
	}while(lmhr==MEM_DID_NOTHING);
    }
    __AROS_FUNC_EXIT
} /* AllocMem */

