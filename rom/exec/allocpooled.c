/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate memory in a pool.
    Lang: english
*/

#include <aros/libcall.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"

#include "exec_debug.h"
#ifndef DEBUG_AllocPooled
#   define DEBUG_AllocPooled 0
#endif
#undef DEBUG
#if DEBUG_AllocPooled
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf



/*****************************************************************************

    NAME */
#include <exec/memory.h>
#include <proto/exec.h>

	AROS_LH2(APTR, AllocPooled,

/*  SYNOPSIS */
	AROS_LHA(APTR,  poolHeader, A0),
	AROS_LHA(IPTR,  memSize,    D0),

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TraceLocation tp = CURRENT_LOCATION("AllocPooled");
    struct Pool *pool = poolHeader + MEMHEADER_TOTAL;

    D(bug("AllocPooled 0x%P memsize %u by \"%s\"\n", poolHeader, memSize, SysBase->ThisTask->tc_Node.ln_Name));

    /* Allocate from the specified pool with flags stored in pool header */
    return InternalAllocPooled(poolHeader, memSize, pool->Requirements, &tp, SysBase);

    AROS_LIBFUNC_EXIT
    
} /* AllocPooled */

