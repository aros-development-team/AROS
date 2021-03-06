/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
*/

#include <asm/cpu.h>
#include <aros/atomic.h>
#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#define __KERNEL_NO_SPINLOCK_PROTOS__
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <exec_platform.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#define D(x)

int Kernel_13_KrnIsSuper();

AROS_LH2(spinlock_t *, KrnSpinTryLock,
        AROS_LHA(spinlock_t *, lock, A0),
        AROS_LHA(ULONG, mode, D0),
        struct KernelBase *, KernelBase, 51, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(0x%p, %08x)\n", __func__, lock, mode));

    if (mode == SPINLOCK_MODE_WRITE)
    {
        /*
        Check if lock->lock equals to SPINLOCK_UNLOCKED. If yes, it will be atomicaly replaced by SPINLOCKF_WRITE and function
        returns 1. Otherwise it copies value of lock->lock into tmp and returns 0.
        */
        if (!compare_and_exchange_long((ULONG*)&lock->lock, SPINLOCK_UNLOCKED, SPINLOCKF_WRITE, NULL))
        {
            D(bug("[Kernel] %s: lock is held (value %08x). Failing to obtain it in WRITE mode...\n", __func__, lock->lock));
            return NULL;
        }
        if (Kernel_13_KrnIsSuper())
        {
            lock->s_Owner = NULL;
        }
        else
        {
            lock->s_Owner = GET_THIS_TASK;
        }
    }
    else
    {
        UBYTE tmp;
        /*
        Check if upper 8 bits of lock->lock are all 0, which means spinlock is not in UPDATING state and it is not
        in the WRITE state. If we manage to obtain it, we set the UPDATING flag. Until we release UPDATING state
        we are free to do whatever we want with the spinlock
        */
        while (!compare_and_exchange_byte((UBYTE*)&lock->block[3], 0, SPINLOCKF_UPDATING >> 24, &tmp))
        {
            /*
            Obtaining lock in UPDATING mode failed. This can have two reasons - either the lock is in WRITE mode, in
            which case we fail and return NULL. Eventually the lock can be in UPDATING mode already, in this case we
            spin for a while...
            */
            if (tmp & (SPINLOCKF_WRITE >> 24))
            {
                D(bug("[Kernel] %s: lock is held in WRITE mode (value %08x). Failing to obtain it in READ mode...\n", __func__, lock->lock));
                return NULL;
            }
            else
            {
                // Tell CPU we are spinning, it shouldn't take long - someone is just updating the lock in READ mode
                asm volatile("pause");

                D(bug("[Kernel] %s: spinning on updating lock ...\n", __func__));
            }
        };
        /*
        At this point we have the spinlock in UPDATING state. So, update readcount field (no worry with atomic add,
        spinlock is for our exclusive use here), and then release it just by setting updating flag to 0

        WARNING: What to do if the 24-bit counter wraps?!
        */
        lock->slock.readcount++;
#if defined(AROS_NO_ATOMIC_OPERATIONS)
        lock->slock.updating = 0;
#else
        __AROS_ATOMIC_AND_L(lock->lock, ~SPINLOCKF_UPDATING);
#endif
    }

    D(bug("[Kernel] %s: lock = %08x\n", __func__, lock->lock));

    return lock;

    AROS_LIBFUNC_EXIT
}
