/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.10  1997/03/07 15:26:48  ldp
    Fix debugging defines

    Revision 1.9  1997/03/07 04:27:24  ldp
    Added debugging

    Revision 1.8  1997/01/01 03:46:05  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.7  1996/12/10 13:51:37  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.6  1996/10/24 15:50:44  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.5  1996/10/19 17:07:24  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.4  1996/08/13 13:55:58  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:05  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <aros/machine.h>
#include "memory.h"
#include <exec/memory.h>
#include <proto/exec.h>

#include "exec_debug.h"
#ifndef DEBUG_AllocPooled
#   define DEBUG_AllocPooled 0
#endif
#if DEBUG_AllocPooled
#   undef DEBUG
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, AllocPooled,

/*  SYNOPSIS */
	AROS_LHA(APTR,  poolHeader, A0),
	AROS_LHA(ULONG, memSize,    D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 118, Exec)

/*  FUNCTION
	Allocate memory out of a private memory pool.

    INPUTS
	poolHeader - Handle of the memory pool
	memSize    - Number of bytes you want to get

    RESULT
	A pointer to the number of bytes you wanted or NULL if the memory
	couldn't be allocated

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePool(), DeletePool(), FreePooled()

    INTERNALS

    HISTORY
	16-10-95    created by M. Fleischer

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    APTR ret;
    struct Pool *pool=(struct Pool *)poolHeader;

    D(bug("AllocPooled $%lx memsize $%lx by \"%s\"\n", poolHeader, memSize, SysBase->ThisTask->tc_Node.ln_Name));

    /* If the memSize is bigger than the ThreshSize allocate seperately. */
    if(memSize>pool->ThreshSize)
    {
	ULONG size;
	struct Block *bl;

	/* Get enough memory for the memory block including the header. */
	size=memSize+BLOCK_TOTAL;
	bl=(struct Block *)AllocMem(size,pool->Requirements);

	/* No memory left */
	if(bl==NULL)
	    return NULL;

	/* Initialize the header */
	bl->Size=size;

	/* Add the block to the BlockList */
	AddHead((struct List *)&pool->BlockList,(struct Node *)&bl->Node);

	/* Set pointer to allocated memory */
	ret=(UBYTE *)bl+BLOCK_TOTAL;
    }else
    {
	struct MemHeader *mh;

	/* Follow the list of MemHeaders */
	mh=(struct MemHeader *)pool->PuddleList.mlh_Head;
	for(;;)
	{
	    /* Are there no more MemHeaders? */
	    if(mh->mh_Node.ln_Succ==NULL)
	    {
		/* Get a new one */
		mh=(struct MemHeader *)
		   AllocMem(pool->PuddleSize+MEMHEADER_TOTAL,pool->Requirements);

		/* No memory left? */
		if(mh==NULL)
		    return NULL;

		/* Initialize new MemHeader */
		mh->mh_First=(struct MemChunk *)((UBYTE *)mh+MEMHEADER_TOTAL);
		mh->mh_First->mc_Next=NULL;
		mh->mh_First->mc_Bytes=pool->PuddleSize;
		mh->mh_Lower=mh->mh_First;
		mh->mh_Upper=(UBYTE *)mh->mh_First+pool->PuddleSize;
		mh->mh_Free=pool->PuddleSize;

		/* And add it to the list */
		AddHead((struct List *)&pool->PuddleList,(struct Node *)&mh->mh_Node);
		/* Fall through to get the memory */
	    }
	    /* Try to get the memory */
	    ret=Allocate(mh,memSize);

	    /* Got it? */
	    if(ret!=NULL)
		break;

	    /* No. Try next MemHeader */
	    mh=(struct MemHeader *)mh->mh_Node.ln_Succ;
	}
	/* Allocate does not clear the memory! */
	if(pool->Requirements&MEMF_CLEAR)
	{
	    ULONG *p=(ULONG *)ret;

	    /* Round up (clearing longs is faster than just bytes) */
	    memSize=(memSize+sizeof(ULONG)-1)/sizeof(ULONG);

	    /* NUL the memory out */
	    while(memSize--)
		*p++=0;
	}
    }
    /* Everything fine */
    return ret;
    AROS_LIBFUNC_EXIT
} /* AllocPooled */

