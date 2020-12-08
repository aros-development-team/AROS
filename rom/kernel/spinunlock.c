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

        AROS_LH1(void, KrnSpinUnLock,

/*  SYNOPSIS */
        AROS_LHA(spinlock_t *, lock, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 53, Kernel)

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
    return;

    AROS_LIBFUNC_EXIT
}
