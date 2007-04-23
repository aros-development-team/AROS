/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    Original version from libnix
    $Id$
*/

#include "pool.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/alib.h>

	APTR LibAllocPooled (

/*  SYNOPSIS */
	APTR  pool,
	ULONG memSize)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	06.12.96 digulla Created after original from libnix

******************************************************************************/
{
#   define poolHeader	((POOL *)pool)
    ULONG *puddle = NULL;

    if (SysBase->LibNode.lib_Version >= 39)
	return (AllocPooled (pool, memSize));

    if (poolHeader != NULL && memSize != 0)
    {
	ULONG size;

	if (poolHeader->ThreshSize > memSize)
	{
	    struct MemHeader * a = (struct MemHeader *)poolHeader->PuddleList.mlh_Head;

	    for(;;)
	    {
		if (a->mh_Node.ln_Succ!=NULL)
		{
		    if (a->mh_Node.ln_Type
			&& (puddle = (ULONG *)Allocate (a, memSize)) != NULL
		    )
			break;

		    a = (struct MemHeader *)a->mh_Node.ln_Succ;
		}
		else
		{
		    size = poolHeader->PuddleSize
			+ sizeof (struct MemHeader)
			+ 2 * sizeof (ULONG);

		    if (!(puddle = (ULONG *)AllocMem (size, poolHeader->MemoryFlags)) )
			return NULL;

		    *puddle ++ = size;

		    a = (struct MemHeader *)puddle;

		    a->mh_Node.ln_Type	 = NT_MEMORY;
		    a->mh_Lower 	 =
			a->mh_First = (struct MemChunk *)(
			    (UBYTE *)a
			    + sizeof (struct MemHeader)
			    + sizeof (UBYTE *)
			);
		    a->mh_First->mc_Next = NULL;
		    a->mh_Free		 =
			a->mh_First->mc_Bytes = poolHeader->PuddleSize;
		    a->mh_Upper 	 = (char *)a->mh_First + a->mh_Free;

		    AddHead ((struct List *)&poolHeader->PuddleList, &a->mh_Node);

		    puddle = (ULONG *)Allocate (a, memSize);

		    break;
		}
	    }

	    /*
		We do have to clear memory here. It may have been dirtied
		by somebody using it beforehand.
	    */
	    if (poolHeader->MemoryFlags & MEMF_CLEAR)
	    {
		ULONG *p = puddle;

		memSize  += 7;
		memSize >>= 3;

		do
		{
		    *p++=0;
		    *p++=0;
		} while (--memSize);
	    }
	}
	else
	{
	    size = memSize + sizeof (struct MinNode) + 2 * sizeof (ULONG);

	    if ((puddle = (ULONG *)AllocMem (size, poolHeader->MemoryFlags)))
	    {
		*puddle ++ = size;

		AddTail ((struct List *)&poolHeader->PuddleList
		    , (struct Node *)puddle
		);

		puddle = (ULONG *)((struct MinNode *)puddle + 1);

		*puddle ++ = 0;
	    }
	}
    }

    return puddle;
} /* LibAllocPooled */

