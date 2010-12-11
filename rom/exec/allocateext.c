/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Allocate memory from a specific MemHeader.
*/

#define MDEBUG 1

#include <aros/debug.h>
#include "exec_intern.h"
#include "memory.h"
#include <exec/alerts.h>
#include <aros/libcall.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

AROS_LH3(APTR, AllocateExt,

/*  SYNOPSIS */
	AROS_LHA(struct MemHeader *, freeList, A0),
	AROS_LHA(ULONG,              byteSize, D0),
	AROS_LHA(ULONG,		     flags,    D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 169, Exec)

/*  FUNCTION
	Allocate memory out of a private region handled by the MemHeader
	structure in a specific way.

    INPUTS
	freeList - Pointer to the MemHeader structure which holds the memory
	byteSize - Number of bytes you want to get
	flags    - Subset of AllocMem() flags telling how to allocate

    RESULT
	A pointer to the number of bytes you wanted or NULL if the memory
	couldn't be allocated

    NOTES
	This function is AROS-specific and private.

    EXAMPLE

    BUGS

    SEE ALSO
	Allocate(), Deallocate()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    APTR res;

    D(bug("[exec] Allocate(0x%p, %u)\n", freeList, byteSize));
    ASSERT_VALID_PTR(freeList);

    /* Zero bytes requested? May return everything ;-). */
    if(!byteSize)
	return NULL;

    /* First round byteSize to a multiple of MEMCHUNK_TOTAL. */
    byteSize=AROS_ROUNDUP2(byteSize,MEMCHUNK_TOTAL);

    /* Is there enough free memory in the list? */
    if(freeList->mh_Free<byteSize)
	return NULL;

    res = stdAlloc(freeList, byteSize, flags, SysBase);

    if ((PrivExecBase(SysBase)->IntFlags & EXECF_MungWall) && res) {
	MUNGE_BLOCK(res, MEMFILL_ALLOC, byteSize);
    }

    return res;

    AROS_LIBFUNC_EXIT
} /* Allocate() */
