/*
    Copyright © 2015-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(spinlock_t *, KrnSpinTryLock,

/*  SYNOPSIS */
        AROS_LHA(spinlock_t *, lock, A0),
        AROS_LHA(ULONG, mode, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 51, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return lock;

    AROS_LIBFUNC_EXIT
}
