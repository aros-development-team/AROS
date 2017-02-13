/*
    Copyright � 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/atomic.h>
#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <utility/hooks.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

#define D(x)

void __spinlock_acquire(spinlock_t *lock)
{
    asm volatile ("lock btsl %1, %0\n\t"
        "jnc 1f\n\t"
        "\n2: pause\n\t"
        "testl %2, %0\n\t"
        "je 2b\n\t"
        "lock btsl %1, %0\n\t"
        "jc 2b\n\t"
        "\n1:"
        :"+m"(lock->lock):"Ir"(SPINLOCKB_WRITE),"r"(SPINLOCKF_WRITE):"memory"
    );
}

int __spinlock_is_locked(spinlock_t *lock)
{
    return (lock->lock & SPINLOCKF_WRITE) != 0;
}

void __spinlock_release(spinlock_t *lock)
{
    lock->lock = 0;
}

static inline int lock_btsl(spinlock_t *lock)
{
    char retval = 0;
    asm volatile("lock btsl %2, %0; setc %1":"+m"(lock->lock),"=r"(retval):"Ir"(SPINLOCKB_WRITE):"memory");
    return retval;
}

static inline int lock_cmpxchg(spinlock_t *lock, ULONG *err)
{
    uint8_t retval;
    uint32_t ret;
    asm volatile("lock cmpxchg %4, %0; setz %1"
        :"+m"(lock->lock),"=r"(retval),"=a"(ret)
        :"2"(SPINLOCK_UNLOCKED),"r"(SPINLOCKF_WRITE)
        :"memory"
    );
    *err = ret;
    return retval;
}
AROS_LH3(spinlock_t *, KrnSpinLock,
	AROS_LHA(spinlock_t *, lock, A1),
	AROS_LHA(struct Hook *, failhook, A0),
	AROS_LHA(ULONG, mode, D0),
	struct KernelBase *, KernelBase, 43, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(0x%p, 0x%p, %08x)\n", __func__, lock, failhook, mode));

    if (mode == SPINLOCK_MODE_WRITE)
    {
        ULONG tmp;
        
        /*
        Check if lock->lock equals to SPINLOCK_UNLOCKED. If yes, it will be atomicaly replaced by SPINLOCKF_WRITE and function
        returns 1. Otherwise it copies value of lock->lock into tmp and returns 0.
        */
        while (!lock_cmpxchg(lock->lock, &tmp)) 
        {
            // Tell CPU we are spinning
            asm volatile("pause");

            // Call failhook if there is any
            if (failhook)
            {
               D(bug("[Kernel] %s: lock-held ... calling fail hook @ 0x%p ...\n", __func__, failhook);)
                CALLHOOKPKT(failhook, (APTR)lock, 0);
            }
            D(bug("[Kernel] %s: spinning on held lock ...\n", __func__);)
        };
    }
    else
    {
        while (lock->lock & SSPINLOCKF_WRITE)
        {
            if (failhook)
            {
                D(bug("[Kernel] %s: write-locked .. calling fail hook @ 0x%p ...\n", __func__, failhook);)
                CALLHOOKPKT(failhook, (APTR)lock, 0);
            }
            D(bug("[Kernel] %s: spinning on write lock ...\n", __func__);)
        };
#if defined(AROS_NO_ATOMIC_OPERATIONS)
        lock->slock.readcount++;
#else
        __AROS_ATOMIC_INC_L(lock->lock);
#endif
    }

    D(bug("[Kernel] %s: lock = %08x\n", __func__, lock->lock));

    return lock;

    AROS_LIBFUNC_EXIT
}
