/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
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
#include <exec/memheaderext.h>
#include <proto/exec.h>

        AROS_LH2(APTR, AllocPooled,

/*  SYNOPSIS */
        AROS_LHA(APTR,  poolHeader, A0),
        AROS_LHA(IPTR,  memSize,    D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 118, Exec)

/*  FUNCTION
        Allocate memory out of a private memory pool. The memory must be
        freed with FreePooled(), or by deallocating the entire pool with
        DeletePool().

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

    struct MemHeaderExt *mhe = (struct MemHeaderExt *)poolHeader;

    /* 0-sized allocation results in returning NULL (API guarantee) */
    if(!memSize)
        return NULL;

    if (IsManagedMem(mhe))
    {
        ULONG poolrequirements = (ULONG)(IPTR)mhe->mhe_MemHeader.mh_First;

        if (mhe->mhe_Alloc)
            return mhe->mhe_Alloc(mhe, memSize, &poolrequirements);
        else
            return NULL;
    }
    else
    {
        struct TraceLocation tp = CURRENT_LOCATION("AllocPooled");
        struct Pool *pool = poolHeader + MEMHEADER_TOTAL;

        D(bug("AllocPooled 0x%p memsize %u by \"%s\"\n", poolHeader, memSize, GET_THIS_TASK->tc_Node.ln_Name);)

        /* Allocate from the specified pool with flags stored in pool header */
        return InternalAllocPooled(poolHeader, memSize, pool->Requirements, &tp, SysBase);
    }

    AROS_LIBFUNC_EXIT
    
} /* AllocPooled */

