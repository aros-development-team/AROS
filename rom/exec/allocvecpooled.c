/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include <aros/debug.h>

AROS_LH2(APTR, AllocVecPooled,
    AROS_LHA(APTR,  pool, D0),
    AROS_LHA(ULONG, size, D1),
    struct ExecBase *, SysBase, 149, Exec)
{
    AROS_LIBFUNC_INIT
    
    IPTR *memory;
    
    if (pool == NULL) return NULL;
    
    size   += sizeof(IPTR);
    memory  = AllocPooled(pool, size);
    
    if (memory != NULL)
    {
        *memory++ = size;
    }

    return memory;

    AROS_LIBFUNC_EXIT
} /* AllocVecPooled() */
