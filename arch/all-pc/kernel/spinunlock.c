/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/atomic.h>
#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

#define D(x)

AROS_LH1(void, KrnSpinUnLock,
	AROS_LHA(spinlock_t *, lock, A0),
	struct KernelBase *, KernelBase, 44, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(0x%p)\n", __func__, lock));

    if (lock->lock & SPINLOCKF_WRITE)
    {
#if defined(AROS_NO_ATOMIC_OPERATIONS)
        lock->lock &= ~SPINLOCKF_WRITE;
#else
        __AROS_ATOMIC_AND_L(lock->lock, ~SPINLOCKF_WRITE);
#endif
    }
    else
    {
#if defined(AROS_NO_ATOMIC_OPERATIONS)
        lock->slock.readcount--;
#else
        __AROS_ATOMIC_DEC_L(lock->lock);
#endif
    }

    D(bug("[Kernel] %s: lock = %08x\n", __func__, lock->lock));

    return;

    AROS_LIBFUNC_EXIT
}
