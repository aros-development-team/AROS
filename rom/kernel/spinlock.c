/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <utility/hooks.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH3(spinlock_t *, KrnSpinLock,

/*  SYNOPSIS */
        AROS_LHA(spinlock_t *, lock, A1),
        AROS_LHA(struct Hook *, failhook, A0),
        AROS_LHA(ULONG, mode, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 52, Kernel)

/*  FUNCTION
        Acquire a spinlock in read or write mode, waiting if it is
        held on supported architectures.

    INPUTS
        lock     - Spinlock to acquire.
        failhook - Optional contention callback on supported targets.
        mode     - SPINLOCK_MODE_READ or SPINLOCK_MODE_WRITE.

    RESULT
        The supplied spinlock pointer.

    NOTES
        The all-pc implementation invokes failhook while waiting for a
        contended lock. ARM implementations do not invoke it.
        The generic fallback returns lock without acquiring it.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnSpinInit(), KrnSpinTryLock(), KrnSpinUnLock()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return lock;

    AROS_LIBFUNC_EXIT
}
