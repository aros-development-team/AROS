/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:10  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang:
*/
#include <aros/libcall.h>
#include "machine.h"
#include "memory.h"

/*****************************************************************************

    NAME */
	#include <exec/memory.h>
	#include <clib/exec_protos.h>

	__AROS_LH3(void,FreePooled,

/*  SYNOPSIS */
	__AROS_LA(APTR, poolHeader,A0),
	__AROS_LA(APTR, memory,    A1),
	__AROS_LA(ULONG,memSize,   D0),

/* LOCATION */
	struct ExecBase *, SysBase, 119, Exec)

/*  FUNCTION
	Free memory allocated out of a private memory pool.

    INPUTS
	poolHeader - Handle of the memory pool
	memory	   - Pointer to the memory
	memSize    - Size of the memory chunk

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePool(), DeletePool(), AllocPooled()

    INTERNALS

    HISTORY
	16-10-95    created by M. Fleischer

******************************************************************************/
{
    __AROS_FUNC_INIT
    struct Pool *pool=(struct Pool *)poolHeader;

    /* If memSize is bigger than the ThreshSize it's allocated seperately. */
    if(memSize>pool->ThreshSize)
    {
	struct Block *bl;

	/* Get pointer to header */
	bl=(struct Block *)((UBYTE *)memory-BLOCK_TOTAL);

	/* Remove it from the list */
	Remove((struct Node *)&bl->Node);

	/* And Free the memory */
	FreeMem(bl,bl->Size);
    }else
    {
	/* Look for the right MemHeader */
	struct MemHeader *mh=(struct MemHeader *)pool->PuddleList.mlh_Head;

	for(;;)
	{
	    /* The memory must be between the two borders */
	    if(memory>=mh->mh_Lower&&memory<mh->mh_Upper)
	    {
		/* Found the MemHeader. Free the memory. */
		Deallocate(mh,memory,memSize);

		/* Is this MemHeader completely free now? */
		if(mh->mh_Free==pool->PuddleSize)
		{
		    /* Yes. Remove it from the list. */
		    Remove(&mh->mh_Node);

		    /* And free it. */
		    FreeMem(mh,pool->PuddleSize+sizeof(struct MemHeader));
		}
		/* All done. */
		break;
	    }
	    /* Try next MemHeader */
	    mh=(struct MemHeader *)mh->mh_Node.ln_Succ;
	}
    }
    __AROS_FUNC_EXIT
} /* FreePooled */

