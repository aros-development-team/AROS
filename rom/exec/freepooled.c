/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free memory allocated by AllocPooled().
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "memory.h"

/*****************************************************************************

    NAME */

	AROS_LH3(void,FreePooled,

/*  SYNOPSIS */
	AROS_LHA(APTR, poolHeader,A0),
	AROS_LHA(APTR, memory,    A1),
	AROS_LHA(IPTR, memSize,   D0),

/* LOCATION */
	struct ExecBase *, SysBase, 119, Exec)

/*  FUNCTION
	Free memory that was allocated out of a private memory pool by
        AllocPooled().

    INPUTS
	poolHeader - Handle of the memory pool
	memory	   - Pointer to the memory
	memSize    - Size of the memory chunk

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePool(), DeletePool(), AllocPooled()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemHeaderExt *mhe = (struct MemHeaderExt *)poolHeader;

    if (IsManagedMem(mhe))
    {
        if (mhe->mhe_Free)
            mhe->mhe_Free(mhe, memory, memSize);
    }
    else
    {
        struct TraceLocation tp = CURRENT_LOCATION("FreePooled");

        InternalFreePooled(poolHeader, memory, memSize, &tp, SysBase);
    }

    AROS_LIBFUNC_EXIT
} /* FreePooled */
