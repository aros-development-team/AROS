/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1996/12/10 13:51:38  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.7  1996/10/24 15:50:45  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.6  1996/10/23 14:13:43  aros
    Use AROS_ALIGN() to align pointers

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

/*****************************************************************************

    NAME */
#include <clib/exec_protos.h>

	AROS_LH2(APTR, AllocVec,

/*  SYNOPSIS */
	AROS_LHA(ULONG, byteSize,     D0),
	AROS_LHA(ULONG, requirements, D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 114, Exec)

/*  FUNCTION
	Allocate some memory from the sytem memory pool with the given
	requirements and without the need to memorize the actual size
	of the block.

    INPUTS
	byteSize     - Number of bytes you want to get
	requirements - Type of memory

    RESULT
	A pointer to the number of bytes you wanted or NULL if the memory
	couldn't be allocated

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeVec()

    INTERNALS

    HISTORY
	8-10-95    created by m. fleischer
       16-10-95    increased portability

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE *ret;

    /* Add room for stored size. */
    byteSize+=AROS_ALIGN(sizeof(ULONG));

    /* Get the memory. */
    ret=(UBYTE *)AllocMem(byteSize,requirements);

    /* If there's not enough memory left return immediately. */
    if(ret==NULL)
	return NULL;

    /* Store size */
    *(ULONG *)ret=byteSize;

    /* return free space */
    return ret+AROS_ALIGN(sizeof(ULONG));
    AROS_LIBFUNC_EXIT
} /* AllocVec */

