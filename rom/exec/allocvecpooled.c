/*
    Copyright � 2003-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <proto/exec.h>
#include <exec/memheaderext.h>
#include "exec_intern.h"
#include <aros/debug.h>

AROS_LH2(APTR, AllocVecPooled,
    AROS_LHA(APTR, pool, D0),
    AROS_LHA(IPTR, size, D1),
    struct ExecBase *, SysBase, 169, Exec)
{
    AROS_LIBFUNC_INIT
    
    struct MemHeaderExt *mhe = (struct MemHeaderExt *)pool;
    
    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_MANAGED)
    {
        ULONG attributes = mhe->mhe_MemHeader.mh_Attributes;

        if (mhe->mhe_Alloc)
            return mhe->mhe_AllocVec(mhe, size, &attributes);
        else
            return NULL;
    }
    else
    {
        IPTR *memory;

        if (pool == NULL) return NULL;

        size   += sizeof(IPTR);
        memory  = AllocPooled(pool, size);

        if (memory != NULL)
        {
            *memory++ = size;
        }

        return memory;
    }

    AROS_LIBFUNC_EXIT
} /* AllocVecPooled() */
