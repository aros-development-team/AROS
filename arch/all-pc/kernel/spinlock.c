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
        while (lock->lock != SPINLOCK_UNLOCKED) 
        {
            if (failhook)
            {
               D(bug("[Kernel] %s: lock-held ... calling fail hook @ 0x%p ...\n", __func__, failhook);)
                CALLHOOKPKT(failhook, (APTR)lock, 0);
            }
            D(bug("[Kernel] %s: spinning on held lock ...\n", __func__);)
        };
#if defined(AROS_NO_ATOMIC_OPERATIONS)
        lock->lock |= SPINLOCKF_WRITE;
#else
        __AROS_ATOMIC_OR_L(lock->lock, SPINLOCKF_WRITE);
#endif
    }
    else
    {
        while (lock->lock &SPINLOCKF_WRITE)
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
