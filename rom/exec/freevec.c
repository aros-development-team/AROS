/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/10/23 14:13:44  aros
    Use AROS_ALIGN() to align pointers

    Revision 1.5  1996/10/19 17:07:26  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.4  1996/08/13 13:56:03  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:12  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <aros/machine.h>
#include "memory.h"

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, FreeVec,

/*  SYNOPSIS */
	__AROS_LHA(APTR, memoryBlock, A1),

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
    if (memoryBlock != NULL)
    {
	*(UBYTE **)&memoryBlock -= AROS_ALIGN(sizeof(ULONG));
	FreeMem (memoryBlock, *((ULONG *)memoryBlock));
    }
    __AROS_FUNC_EXIT
} /* FreeVec */
