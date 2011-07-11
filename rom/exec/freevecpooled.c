/*
    Copyright © 2003-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

AROS_LH2(void, FreeVecPooled,
    AROS_LHA(APTR, pool,   D0),
    AROS_LHA(APTR, memory, D1),
    struct ExecBase *, SysBase, 170, Exec)
{
    AROS_LIBFUNC_INIT
    
    if (memory != NULL)
    {
        IPTR *real = (IPTR *) memory;
        IPTR size  = *--real;

        FreePooled(pool, real, size);
    }
    
    AROS_LIBFUNC_EXIT
} /* FreeVecPooled() */
