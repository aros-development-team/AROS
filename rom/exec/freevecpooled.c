/*
    Copyright � 2003-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <proto/exec.h>
#include <exec/memheaderext.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(void, FreeVecPooled,

/*  SYNOPSIS */
        AROS_LHA(APTR, poolHeader, A0),
        AROS_LHA(APTR, memory, A1),

/* LOCATION */
        struct ExecBase *, SysBase, 170, Exec)

/*  FUNCTION
        Free memory that was allocated out of a private memory pool by
        AllocVecPooled().

    INPUTS
        poolHeader - Handle of the memory pool
        memory     - Pointer to the memory

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CreatePool(), DeletePool(), AllocVecPooled()

    INTERNALS
        In AROS memory allocated from pool remembers where it came from.
        Because of this poolHeader is effectively ignored and is present
        only for compatibility reasons. However, do not rely on this! For
        other operating systems of Amiga(tm) family this is not true!

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct MemHeaderExt *mhe = (struct MemHeaderExt *)poolHeader;

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_MANAGED)
    {
        if (mhe->mhe_FreeVec)
            mhe->mhe_FreeVec(mhe, memory);
    }
    else
    {
        if (memory != NULL)
        {
            IPTR *real = (IPTR *) memory;
            IPTR size  = *--real;

            FreePooled(poolHeader, real, size);
        }
    }
    AROS_LIBFUNC_EXIT
} /* FreeVecPooled() */
