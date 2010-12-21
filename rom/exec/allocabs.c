/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate memory at address
    Lang: english
*/

/* Needed for mungwall macros to work */
#define MDEBUG 1

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "memory.h"

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, AllocAbs,

/*  SYNOPSIS */
	AROS_LHA(ULONG, byteSize, D0),
	AROS_LHA(APTR,  location, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 34, Exec)

/*  FUNCTION
	Allocate some memory from the system memory pool at a given address.

    INPUTS
	byteSize - Number of bytes you want to get
	location - Where you want to get the memory

    RESULT
	A pointer to some memory including the requested bytes or NULL if
	the memory couldn't be allocated

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeMem()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemHeader *mh;
    IPTR origSize = byteSize;
    APTR ret = NULL;
    
    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* Make room for mungwall if needed */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_MungWall)
    {
    	location -= MUNGWALL_SIZE + MUNGWALLHEADER_SIZE;
        byteSize += MUNGWALL_SIZE * 2 + MUNGWALLHEADER_SIZE;
    }

    /* Protect the memory list from access by other tasks. */
    Forbid();

    /* Loop over MemHeader structures */
    ForeachNode(&SysBase->MemList, mh)
    {
        if (mh->mh_Lower <= location && mh->mh_Upper > location)
            break;
    }
    
    /* If no header was found which matched the requirements, just give up. */
    if (!mh->mh_Node.ln_Succ)
    {
        Permit();
        return NULL;
    }

    /* If the header is managed, let the manager handle the request.  */    
    if (mh->mh_Attributes & MEMF_MANAGED)
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

        if (mhe->mhe_AllocAbs)
	    ret = mhe->mhe_AllocAbs(mhe, byteSize, location);
    }
    else
    {
        struct MemChunk *p1, *p2, *p3, *p4;
        
        /* Align size to the requirements */
        byteSize += (IPTR)location&(MEMCHUNK_TOTAL - 1);
        byteSize  = (byteSize + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1);
        
        /* Align the location as well */
        location=(APTR)((IPTR)location & ~(MEMCHUNK_TOTAL-1));
        
        /* Start and end(+1) of the block */
        p3=(struct MemChunk *)location;
        p4=(struct MemChunk *)((UBYTE *)p3+byteSize);
        
        /*
            The free memory list is only single linked, i.e. to remove
            elements from the list I need the node's predessor. For the
            first element I can use freeList->mh_First instead of a real
            predecessor.
        */
        p1 = (struct MemChunk *)&mh->mh_First;
        p2 = p1->mc_Next;

        /* Follow the list to find a chunk with our memory. */
        while (p2 != NULL)
        {
#if !defined(NO_CONSISTENCY_CHECKS)
	    /*
	     * Memory list consistency checks.
	     * 1. Check alignment restrictions
	     */
            if (((IPTR)p2|(IPTR)p2->mc_Bytes) & (MEMCHUNK_TOTAL-1))
	    {
		if (SysBase && SysBase->DebugAROSBase)
		{
		    bug("[MM] Chunk allocator error\n");
		    bug("[MM] Attempt to allocate %lu bytes at 0x%p from MemHeader 0x%p\n", origSize, location, mh);
		    bug("[MM] Misaligned chunk at 0x%p (%u bytes)\n", p2, p2->mc_Bytes);

		    Alert(AN_MemCorrupt|AT_DeadEnd);
		}
		return NULL;
	    }

	    /* 2. Check against overlapping blocks */
	    if (p2->mc_Next && ((UBYTE *)p2 + p2->mc_Bytes >= (UBYTE *)p2->mc_Next))
	    {
		if (SysBase && SysBase->DebugAROSBase)
		{
		    bug("[MM] Chunk allocator error\n");
		    bug("[MM] Attempt to allocate %lu bytes at 0x%p from MemHeader 0x%p\n", origSize, location, mh);
		    bug("[MM] Overlapping chunks 0x%p (%u bytes) and 0x%p (%u bytes)\n", p2, p2->mc_Bytes, p2->mc_Next, p2->mc_Next->mc_Bytes);
		
		    Alert(AN_MemCorrupt|AT_DeadEnd);
		}
		return NULL;
	    }
#endif

            /* Found a chunk that fits? */
            if((UBYTE *)p2+p2->mc_Bytes>=(UBYTE *)p4&&p2<=p3)
            {
                /* Check if there's memory left at the end. */
                if((UBYTE *)p2+p2->mc_Bytes!=(UBYTE *)p4)
                {
                    /* Yes. Add it to the list */
                    p4->mc_Next  = p2->mc_Next;
                    p4->mc_Bytes = (UBYTE *)p2+p2->mc_Bytes-(UBYTE *)p4;
                    p2->mc_Next  = p4;
                }

                /* Check if there's memory left at the start. */
                if(p2!=p3)
                    /* Yes. Adjust the size */
                    p2->mc_Bytes=(UBYTE *)p3-(UBYTE *)p2;
                else
                    /* No. Skip the old chunk */
                    p1->mc_Next=p2->mc_Next;
    
                /* Adjust free memory count */
                mh->mh_Free-=byteSize;

                /* Return the memory */
                ret = p3;
                break;
            }
            /* goto next chunk */
        
            p1=p2;
            p2=p2->mc_Next;
        }
    }

    Permit();

    return MungWall_Build(ret, origSize + location - ret, 0, SysBase);
    
    AROS_LIBFUNC_EXIT
} /* AllocAbs */

