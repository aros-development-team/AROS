/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    Original version from libnix
    $Id$

    Desc:
    Lang: english
*/
#include "pool.h"
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
#include <clib/alib_protos.h>

	void LibFreePooled (

/*  SYNOPSIS */
	APTR  pool,
	APTR  memory,
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
#   define poolHeader ((POOL *)pool)

    if (SysBase->LibNode.lib_Version >= 39)
    {
	FreePooled (poolHeader, memory, memSize);
	return;
    }

    if (poolHeader!=NULL && memory!=NULL)
    {
	ULONG size,
	    * puddle = (ULONG *)((struct MinNode *)memory - 1) - 1;

	if (poolHeader->ThreshSize>memSize)
	{
	    struct MemHeader * a = (struct MemHeader *)&poolHeader->PuddleList.mlh_Head;

	    for(;;)
	    {
		a = (struct MemHeader *)a->mh_Node.ln_Succ;

		if (a->mh_Node.ln_Succ == NULL)
		    return;

		if (a->mh_Node.ln_Type && memory >= a->mh_Lower
		    && memory < a->mh_Upper
		)
		    break;
	    }

	    Deallocate (a, memory, memSize);

	    if (a->mh_Free != poolHeader->PuddleSize)
		return;

	    puddle = (ULONG *)&a->mh_Node;
	}

	Remove ((struct Node *)puddle);

	size = *--puddle;
	FreeMem (puddle, size);
    }
} /* LibFreePooled */

