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
        /*
        Unlocking READ mode spinlock means we need to put it into UDPATING state, decrement counter and unlock from
        UPDATING state.
        */
        while (!compare_and_exchange_byte((UBYTE*)&lock->block[3], 0, SPINLOCKF_UPDATING >> 24, NULL))
        {
            // Tell CPU we are spinning
            asm volatile("pause");
        }

        // Just in case someone tries to unlock already unlocked stuff
        if (lock->slock.readcount != 0)
        {
            lock->slock.readcount--;
        }
#if defined(AROS_NO_ATOMIC_OPERATIONS)
        lock->slock.updating = 0;
#else
        __AROS_ATOMIC_AND_L(lock->lock, ~SPINLOCKF_UPDATING);
#endif
    }

    D(bug("[Kernel] %s: lock = %08x\n", __func__, lock->lock));

    return;

    AROS_LIBFUNC_EXIT
}
