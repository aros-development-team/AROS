/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1997/01/01 03:46:08  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.7  1996/12/10 13:51:43  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.6  1996/10/24 15:50:47  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.5  1996/10/19 17:07:25  aros
    Include <aros/machine.h> instead of machine.h

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
#include <aros/machine.h>
#include "memory.h"
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, DeletePool,

/*  SYNOPSIS */
	AROS_LHA(APTR, poolHeader, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 117, Exec)

/*  FUNCTION
	Delete a pool including all it's memory.

    INPUTS
	poolHeader - The pool allocated with CreatePool() or NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePool(), AllocPooled(), FreePooled()

    INTERNALS

    HISTORY
	16-10-95    created by m. fleischer

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Pool *pool=(struct Pool *)poolHeader;

    /* It is legal to DeletePool(NULL) */
    if(pool!=NULL)
    {
	void *p;
	struct Block *bl;
	ULONG size;

	/* Calculate the total size of a puddle including header. */
	size=pool->PuddleSize+MEMHEADER_TOTAL;
	/* Free the list of puddles */
	while((p=RemHead((struct List *)&pool->PuddleList))!=NULL)
	    FreeMem(p,size);

	/* Free the list of single Blocks */
	while((bl=(struct Block *)RemHead((struct List *)&pool->BlockList))!=NULL)
	    FreeMem(bl,bl->Size);

	FreeMem(pool,sizeof(struct Pool));
    }
    AROS_LIBFUNC_EXIT
} /* DeletePool */

