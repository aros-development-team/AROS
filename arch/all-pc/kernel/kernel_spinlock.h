#ifndef KERNEL_SPINLOCK_H_
#define KERNEL_SPINLOCK_H_
/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.

    Desc: Atomic steps of the kernel spinlock, x86 version.
          See rom/kernel/kernel_spinlock.h for what each one must do.
*/

#include <asm/cpu.h>
#include <aros/types/spinlock_s.h>

#define KRN_SPINLOCK_ATOMICS

/* The task running on this CPU */
#if defined(__AROSEXEC_SMP__)
#include "kernel_intern.h"
#include "tls.h"
#define krnSpinThisTask()       TLS_GET(Sched.RunningTask)
#else
#define krnSpinThisTask()       (SysBase->ThisTask)
#endif

#define SPINLOCK_BUSY_FOR_READ  (SPINLOCKF_WRITE | SPINLOCKF_UPDATING)

static inline BOOL krnSpinTryWrite(spinlock_t *lock)
{
    return compare_and_exchange_long((ULONG *)&lock->lock, SPINLOCK_UNLOCKED, SPINLOCKF_WRITE, NULL) ? TRUE : FALSE;
}

/*
 * One compare and exchange on the whole word. The word is never left in an
 * intermediate state, so a reader that is switched out here cannot keep the
 * other users of the lock waiting until it runs again.
 */
static inline BOOL krnSpinTryRead(spinlock_t *lock)
{
    ULONG old = lock->lock;

    for (;;)
    {
        ULONG found;

        if (old & SPINLOCK_BUSY_FOR_READ)
            return FALSE;
        if (compare_and_exchange_long((ULONG *)&lock->lock, old, old + 1, &found))
            return TRUE;
        /* Another reader came or went; try again with what is there now */
        old = found;
    }
}

static inline void krnSpinReleaseWrite(spinlock_t *lock)
{
    compare_and_exchange_long((ULONG *)&lock->lock, SPINLOCKF_WRITE, SPINLOCK_UNLOCKED, NULL);
}

static inline void krnSpinReleaseRead(spinlock_t *lock)
{
    ULONG old = lock->lock;

    for (;;)
    {
        ULONG found;

        /* Not held for reading: leave it alone */
        if ((old & SPINLOCK_BUSY_FOR_READ) || !(old & 0x00ffffff))
            return;
        if (compare_and_exchange_long((ULONG *)&lock->lock, old, old - 1, &found))
            return;
        old = found;
    }
}

static inline void krnSpinPause(void)
{
    asm volatile("pause");
}

#endif /* KERNEL_SPINLOCK_H_ */
