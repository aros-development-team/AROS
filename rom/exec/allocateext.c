/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Allocate memory from a specific MemHeader.
*/

#define MDEBUG 1

#include <aros/debug.h>
#include "exec_intern.h"
#include "memory.h"
#include <exec/alerts.h>
#include <aros/libcall.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

AROS_LH3(APTR, AllocateExt,

/*  SYNOPSIS */
	AROS_LHA(struct MemHeader *, freeList, A0),
	AROS_LHA(ULONG,              byteSize, D0),
	AROS_LHA(ULONG,		     requirements, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 169, Exec)

/*  FUNCTION
	Allocate memory out of a private region handled by the MemHeader
	structure in a specific way.

    INPUTS
	freeList     - Pointer to the MemHeader structure which holds the memory
	byteSize     - Number of bytes you want to get
	requirements - Subset of AllocMem() flags telling how to allocate

    RESULT
	A pointer to the number of bytes you wanted or NULL if the memory
	couldn't be allocated

    NOTES
	This function is AROS-specific and private.

    EXAMPLE

    BUGS

    SEE ALSO
	Allocate(), Deallocate()

    INTERNALS
	Currently only MEMF_REVERSE flag is supported in requirements
	parameter. Other flags are silently ignored. It is unknown when and
	whether this will be changed. Remember, i repeat, this function is
	PRIVATE!

	This function can be safely called statically with SysBase = NULL.

******************************************************************************/
{
    AROS_LIBFUNC_INIT
 
    struct MemChunk *mc=NULL, *p1, *p2;

    D(bug("[exec] AllocateExt(0x%p, %u, 0x%08X)\n", freeList, byteSize, requirements));
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
        elements from the list I need node's predessor. For the
        first element I can use freeList->mh_First instead of a real predessor.
    */
    p1 = (struct MemChunk *)&freeList->mh_First;
    p2 = p1->mc_Next;

    /* Is there anything in the list? */
    if (p2 != NULL)
    {
        /* Then follow it */
        for (;;)
        {
#if !defined(NO_CONSISTENCY_CHECKS)
            /* Consistency check: Check alignment restrictions */
            if (((IPTR)p2|(IPTR)p2->mc_Bytes) & (MEMCHUNK_TOTAL-1))
	    {
		if (SysBase)
		{
		    bug("[MM] Chunk allocator error\n");
		    bug("[MM] Attempt to allocate %u bytes from MemHeader 0x%p\n", byteSize, freeList);
		    bug("[MM] Misaligned chunk at 0x%p (%u bytes)\n", p2, p2->mc_Bytes);

		    Alert(AN_MemCorrupt|AT_DeadEnd);
		}
		return NULL;
	    }
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
	    {
		if (SysBase)
		{
		    bug("[MM] Chunk allocator error\n");
		    bug("[MM] Attempt to allocate %u bytes from MemHeader 0x%p\n", byteSize, freeList);
		    bug("[MM] Overlapping chunks 0x%p (%u bytes) and 0x%p (%u bytes)\n", p1, p1->mc_Bytes, p2, p2->mc_Bytes);

		    Alert(AN_MemCorrupt|AT_DeadEnd);
		}
		return NULL;
	    }
#endif
        }
        
        /* Something found? */
        if (mc != NULL)
        {
            /*
                Remember: if MEMF_REVERSE is set
                p1 and p2 are now invalid.
            */
            p1=mc;
            p2=p1->mc_Next;

            /* Remove the block from the list and return it. */
            if(p2->mc_Bytes == byteSize)
            {
                /* Fits exactly. Just relink the list. */
                p1->mc_Next = p2->mc_Next;
                mc          = p2;
            }
            else
            {
                if(requirements & MEMF_REVERSE)
                {
                    /* Return the last bytes. */
                    p1->mc_Next=p2;
                    mc=(struct MemChunk *)((UBYTE *)p2+p2->mc_Bytes-byteSize);
                }
                else
                {
                    /* Return the first bytes. */
                    p1->mc_Next=(struct MemChunk *)((UBYTE *)p2+byteSize);
                    mc=p2;
                }
                
                p1           = p1->mc_Next;
                p1->mc_Next  = p2->mc_Next;
                p1->mc_Bytes = p2->mc_Bytes-byteSize;
            }
            
            freeList->mh_Free -= byteSize;
        }
    }

    return mc;

    AROS_LIBFUNC_EXIT
}
