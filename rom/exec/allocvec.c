/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate some memory.
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include "memory.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

        AROS_LH2(APTR, AllocVec,

/*  SYNOPSIS */
        AROS_LHA(IPTR,  byteSize,     D0),
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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE *ret;

    /* 0-sized allocation results in returning NULL (API guarantee) */
    if(!byteSize)
        return NULL;

    /* Add room for stored size. */
    byteSize+=AROS_ALIGN(sizeof(IPTR));

    /* Get the memory. */
    ret=(UBYTE *)AllocMem(byteSize,requirements);

    /* If there's not enough memory left return immediately. */
    if(ret==NULL)
        return NULL;

    /* Store size */
    *(IPTR *)ret=byteSize;

    /* return free space */
    return ret+AROS_ALIGN(sizeof(IPTR));
    AROS_LIBFUNC_EXIT
} /* AllocVec */

