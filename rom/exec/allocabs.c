/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1996/11/21 10:49:48  aros
    Created macros AROS_SLIB_ENTRY() for assembler files, too, to solve naming
    problems.

    The #includes in the header *must* begin in the first column. Otherwise
    makedepend will ignore them (GCC works, though).

    Removed a couple of Logs

    Revision 1.7  1996/10/24 15:50:43  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.6  1996/10/19 17:07:23  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.5  1996/09/13 17:51:21  digulla
    Use IPTR

    Revision 1.4  1996/08/13 13:55:57  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:04  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <aros/machine.h>
#include <clib/exec_protos.h>
#include "memory.h"

/*****************************************************************************

    NAME */
	#include <exec/memory.h>
	#include <clib/exec_protos.h>

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

    HISTORY
       17-10-95    created by M. Fleischer

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MemChunk *p1,*p2,*p3,*p4;
    struct MemHeader *mh;

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* Align size to the requirements */
    byteSize+=(IPTR)location&(MEMCHUNK_TOTAL-1);
    byteSize=(byteSize+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1);

    /* Align the location as well */
    location=(APTR)((IPTR)location&~(MEMCHUNK_TOTAL-1));

    /* Start and end(+1) of the block */
    p3=(struct MemChunk *)location;
    p4=(struct MemChunk *)((UBYTE *)p3+byteSize);

    /* Protect the memory list from access by other tasks. */
    Forbid();

    /* Loop over MemHeader structures */
    mh=(struct MemHeader *)SysBase->MemList.lh_Head;
    while(mh->mh_Node.ln_Succ)
    {
	/* Test if the memory belongs to this MemHeader. */
	if(mh->mh_Lower<=location&&mh->mh_Upper>location)
	{
	    /*
		The free memory list is only single linked, i.e. to remove
		elements from the list I need the node's predessor. For the
		first element I can use freeList->mh_First instead of a real
		predessor.
	    */
	    p1=(struct MemChunk *)&mh->mh_First;
	    p2=p1->mc_Next;

	    /* Follow the list to find a chunk with our memory. */
	    while(p2!=NULL)
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
		/* Found a chunk that fits? */
		if((UBYTE *)p2+p2->mc_Bytes>=(UBYTE *)p4&&p2<=p3)
		{
		    /* Check if there's memory left at the end. */
		    if((UBYTE *)p2+p2->mc_Bytes!=(UBYTE *)p4)
		    {
			/* Yes. Add it to the list */
			p4->mc_Next=p2->mc_Next;
			p4->mc_Bytes=(UBYTE *)p2+p2->mc_Bytes-(UBYTE *)p4;
			p2->mc_Next=p4;
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
		    Permit();
		    return p3;
		}
		/* goto next chunk */
		p1=p2;
		p2=p2->mc_Next;
	    }
	    /* The MemHeader didn't have the memory */
	    break;
	}
	/* Test next MemHeader */
	mh=(struct MemHeader *)mh->mh_Node.ln_Succ;
    }
    /* There's nothing we could do */
    Permit();
    return NULL;
    AROS_LIBFUNC_EXIT
} /* AllocAbs */

