/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:55:58  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:05  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include "machine.h"
#include "memory.h"

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH2(APTR, AllocVec,

/*  SYNOPSIS */
	__AROS_LHA(ULONG, byteSize,     D0),
	__AROS_LHA(ULONG, requirements, D1),

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
    __AROS_FUNC_INIT

    UBYTE *ret;

    /* Add room for stored size. */
    byteSize+=ALLOCVEC_TOTAL;

    /* Get the memory. */
    ret=(UBYTE *)AllocMem(byteSize,requirements);

    /* If there's not enough memory left return immediately. */
    if(ret==NULL)
	return NULL;

    /* Store size */
    *(ULONG *)ret=byteSize;

    /* return free space */
    return ret+ALLOCVEC_TOTAL;
    __AROS_FUNC_EXIT
} /* AllocVec */

