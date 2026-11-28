/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_spinlock.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(void, KrnSpinUnLock,

/*  SYNOPSIS */
        AROS_LHA(spinlock_t *, lock, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 53, Kernel)

/*  FUNCTION
        Release one read or write acquisition of a spinlock.

    INPUTS
        lock - Spinlock to release.

    RESULT

    NOTES
        The behaviour is the same on every port with real spinlocks.
        Releasing a lock that is not held leaves it unchanged.

        Ports without real spinlocks do not modify the lock.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnSpinInit(), KrnSpinLock(), KrnSpinTryLock()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(KRN_SPINLOCK_ATOMICS)
    if (lock->lock & SPINLOCKF_WRITE)
    {
        /* The owner goes first: once the word is free it is someone else's */
        lock->s_Owner = NULL;
        krnSpinReleaseWrite(lock);
    }
    else
        krnSpinReleaseRead(lock);
#endif

    return;

    AROS_LIBFUNC_EXIT
}
