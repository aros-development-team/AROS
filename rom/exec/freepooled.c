/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/19 17:07:26  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.4  1996/08/13 13:56:02  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:11  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <aros/machine.h>
#include "memory.h"

/*****************************************************************************

    NAME */
	#include <exec/memory.h>
	#include <clib/exec_protos.h>

	__AROS_LH3(void,FreePooled,

/*  SYNOPSIS */
	__AROS_LHA(APTR, poolHeader,A0),
	__AROS_LHA(APTR, memory,    A1),
	__AROS_LHA(ULONG,memSize,   D0),

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

