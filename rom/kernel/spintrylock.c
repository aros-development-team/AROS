/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_spinlock.h>

#if defined(KRN_SPINLOCK_ATOMICS)
#include <exec/tasks.h>
#include <proto/exec.h>

/* Called directly: it takes no base, and the lock is used before the base exists */
extern int AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)(void);
#define krnSpinIsSuper()        AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)()
#endif

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
        The behaviour is the same on every port with real spinlocks. A
        write lock taken by a task records the task as the owner; see
        KrnSpinLock(). A read lock is refused only while the lock is held
        for writing.

        Ports without real spinlocks return lock without acquiring it.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnSpinInit(), KrnSpinLock(), KrnSpinUnLock()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(KRN_SPINLOCK_ATOMICS)
    if (mode == SPINLOCK_MODE_WRITE)
    {
        if (!krnSpinTryWrite(lock))
            return NULL;

        lock->s_Owner = krnSpinIsSuper() ? NULL : krnSpinThisTask();
    }
    else if (!krnSpinTryRead(lock))
        return NULL;
#endif

    return lock;

    AROS_LIBFUNC_EXIT
}
