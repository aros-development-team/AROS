/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

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
        Attempt to acquire a spinlock in read or write mode without
        waiting for a current lock holder to release it.

    INPUTS
        lock - Spinlock to acquire.
        mode - SPINLOCK_MODE_READ or SPINLOCK_MODE_WRITE.

    RESULT
        Lock pointer if acquired; NULL if the requested mode cannot
        be obtained on an implementation with spinlock support.

    NOTES
        The generic fallback returns lock without acquiring it.
        The implementation and availability are architecture-specific.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnSpinInit(), KrnSpinLock(), KrnSpinUnLock()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return lock;

    AROS_LIBFUNC_EXIT
}
