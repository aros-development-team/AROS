/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1997/01/01 03:46:08  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.7  1996/12/10 13:51:42  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.6  1996/10/24 15:50:47  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.5  1996/10/19 17:09:02  aros
    Include <aros/machine.h> over "machine.h"
    Fixed a type in the docs

    Revision 1.4  1996/08/13 13:56:00  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:09  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <proto/alib.h>
#include <aros/machine.h>
#include "memory.h"
#include <exec/memory.h>
#include <proto/exec.h>

#define NEWLIST(l) \
((l)->lh_Head=(struct Node *)&(l)->lh_Tail, \
 (l)->lh_Tail=NULL,                         \
 (l)->lh_TailPred=(struct Node *)(l))

/*****************************************************************************

    NAME */

	AROS_LH3(APTR, CreatePool,

/*  SYNOPSIS */
	AROS_LHA(ULONG, requirements, D0),
	AROS_LHA(ULONG, puddleSize,   D1),
	AROS_LHA(ULONG, threshSize,   D2),

/*  LOCATION */
	struct ExecBase *, SysBase, 116, Exec)

/*  FUNCTION
	Create a private pool for memory allocations.

    INPUTS
	requirements - The type of the memory
	puddleSize   - The number of bytes that the pool expands
		       if it is too small.
	threshSize   - Allocations beyond the threshSize are given
		       directly to the system. threshSize must be
		       smaller than the puddleSize.

    RESULT
	A handle for the memory pool or NULL if the pool couldn't
	be created

    NOTES

    EXAMPLE
	\* Get the handle to a private memory pool *\
	po=CreatePool(MEMF_ANY,16384,8192);
	if(po!=NULL)
	{
	    \* Use the pool *\
	    UBYTE *mem1,*mem2;
	    mem1=AllocPooled(po,1000);
	    mem2=AllocPooled(po,2000);
	    \* Do something with the memory... *\

	    \* Free everything at once *\
	    DeletePool(po);
	}

    BUGS

    SEE ALSO
	DeletePool(), AllocPooled(), FreePooled()

    INTERNALS

    HISTORY
	16-10-95    created by m. fleischer

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Pool *pool=NULL;

    /* puddleSize must not be smaller than threshSize */
    if(puddleSize>=threshSize)
    {
	/* Round puddleSize up to be a multiple of MEMCHUNK_TOTAL. */
	puddleSize=(puddleSize+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1);

	/*
	    Allocate memory for the Pool structure using the same requirements
	    as for the whole pool (to make it shareable, residentable or
	    whatever is needed).
	*/
	pool=(struct Pool *)AllocMem(sizeof(struct Pool),requirements);
	if(pool!=NULL)
	{
	    /* Clear the lists */
	    NEWLIST((struct List *)&pool->PuddleList);
	    NEWLIST((struct List *)&pool->BlockList );

	    /* Set the rest */
	    pool->Requirements=requirements;
	    pool->PuddleSize  =puddleSize;
	    pool->ThreshSize  =threshSize;
	}
    }
    return pool;
    AROS_LIBFUNC_EXIT
} /* CreatePool */
