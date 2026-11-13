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
        /*
        Add one reader with a single compare and exchange (see KrnSpinLock). Fail if the lock is in WRITE mode.

        WARNING: What to do if the 24-bit counter wraps?!
        */
        for (;;)
        {
            ULONG old = lock->lock;

            if (old & SPINLOCKF_WRITE)
            {
                D(bug("[Kernel] %s: lock is held in WRITE mode (value %08x). Failing to obtain it in READ mode...\n", __func__, lock->lock));
                return NULL;
            }
            if (!(old & SPINLOCKF_UPDATING) &&
                compare_and_exchange_long((ULONG*)&lock->lock, old, old + 1, NULL))
                break;

            asm volatile("pause");
        }
    }

    D(bug("[Kernel] %s: lock = %08x\n", __func__, lock->lock));

    return lock;

    AROS_LIBFUNC_EXIT
}
