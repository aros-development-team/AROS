/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:10  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include "machine.h"
#include "memory.h"

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, FreeVec,

/*  SYNOPSIS */
	__AROS_LA(APTR, memoryBlock, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 115, Exec)

/*  FUNCTION
	Free some memory allocated with allocvec.

    INPUTS
	memoryBlock - The memory to be freed. It is safe to free a NULL pointer.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocVec()

    INTERNALS

    HISTORY
	15-10-95    created by m. fleischer

******************************************************************************/
{
    __AROS_FUNC_INIT

    /* If there's nothing to free do nothing. */
    if(memoryBlock!=NULL)
    {
	*(UBYTE **)&memoryBlock-=ALLOCVEC_TOTAL;
	FreeMem(memoryBlock,*(ULONG *)memoryBlock);
    }
    __AROS_FUNC_EXIT
} /* FreeVec */
