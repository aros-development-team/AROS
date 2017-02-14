/*
    Copyright � 2017, The AROS Development Team. All rights reserved.
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

static inline int lock_cmpxchg_b(spinlock_t *lock, UBYTE *err)
{
    UBYTE retval;
    UBYTE ret;
    asm volatile("lock cmpxchgb %b4, %0; setz %1"
        :"+m"(lock->block[3]),"=r"(retval),"=a"(ret)
        :"2"(0),"d"(SPINLOCKF_UPDATING >> 24)
        :"memory"
    );
    *err = ret;
    return retval;
}

AROS_LH1(void, KrnSpinUnLock,
	AROS_LHA(spinlock_t *, lock, A0),
	struct KernelBase *, KernelBase, 44, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(0x%p)\n", __func__, lock));

    // Warning: replace this if of WRITE mode spinlock with proper cmpxchg...
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
        UBYTE tmp;
        /*
        Unlocking READ mode spinlock means we need to put it into UDPATING state, decrement counter and unlock from
        UPDATING state.
        */
        while (!lock_cmpxchg_b(lock, &tmp))
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
