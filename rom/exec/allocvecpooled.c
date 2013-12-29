/*
    Copyright � 2003-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include "exec_intern.h"
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

        AROS_LH2(APTR, AllocVecPooled,

/*  SYNOPSIS */
        AROS_LHA(APTR, poolHeader, A0),
        AROS_LHA(IPTR, memSize, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 169, Exec)

/*  FUNCTION
        Allocate memory out of a private memory pool and remember the size.
        The memory must be freed with FreeVecPooled(), or by deallocating
        the entire pool with DeletePool().

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
        CreatePool(), DeletePool(), FreeVecPooled()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct MemHeaderExt *mhe = (struct MemHeaderExt *)poolHeader;
    
    if (IsManagedMem(mhe))
    {
        ULONG attributes = (ULONG)(IPTR)mhe->mhe_MemHeader.mh_First;

        if (mhe->mhe_Alloc)
            return mhe->mhe_AllocVec(mhe, memSize, &attributes);
        else
            return NULL;
    }
    else
    {
        IPTR *memory;

        if (poolHeader == NULL) return NULL;

        memSize   += sizeof(IPTR);
        memory  = AllocPooled(poolHeader, memSize);

        if (memory != NULL)
        {
            *memory++ = memSize;
        }

        return memory;
    }

    AROS_LIBFUNC_EXIT
} /* AllocVecPooled() */
