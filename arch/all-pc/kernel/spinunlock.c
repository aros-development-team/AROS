/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
*/

#include <asm/cpu.h>
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
        struct KernelBase *, KernelBase, 53, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(0x%p)\n", __func__, lock));

    lock->s_Owner = NULL;
    /*
    use cmpxchg - expect SPINLOCKF_WRITE and replace it with 0 (unlocking the spinlock), if that succeeded, the lock
    was in WRITE mode and is now free. If that was not the case, continue with unlocking READ mode spinlock
    */
    if (!compare_and_exchange_long((ULONG*)&lock->lock, SPINLOCKF_WRITE, 0, NULL))
    {
        ULONG old;

        /*
        Unlocking READ mode spinlock: take one reader off with a single compare and exchange (see KrnSpinLock).
        A lock without readers is left alone, just in case someone tries to unlock already unlocked stuff.
        */
        do
        {
            old = lock->lock;
        } while ((old & ~(SPINLOCKF_WRITE | SPINLOCKF_UPDATING)) &&
                 !compare_and_exchange_long((ULONG*)&lock->lock, old, old - 1, NULL));
    }

    D(bug("[Kernel] %s: lock = %08x\n", __func__, lock->lock));

    return;

    AROS_LIBFUNC_EXIT
}
