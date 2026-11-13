/*
    Copyright (C) 2015-2020, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(int, KrnSpinIsLocked,

/*  SYNOPSIS */
        AROS_LHA(spinlock_t *, lock, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 50, Kernel)

/*  FUNCTION
        Report whether a spinlock is locked.

    INPUTS
        lock - Pointer to the spinlock to inspect.

    RESULT
        Nonzero if the spinlock is locked; zero if it is unlocked.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return 0;

    AROS_LIBFUNC_EXIT
}
