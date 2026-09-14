/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <asm/cpu.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH1(void, KrnSpinUnLock,
        AROS_LHA(spinlock_t *, lock, A0),
        struct KernelBase *, KernelBase, 53, Kernel)
{
    AROS_LIBFUNC_INIT

    if (lock->lock == 0)
        return;

    /* stlr publishes the critical section before the clear; the dsb keeps
     * the sev from overtaking it and leaving a waiter parked. */
    if (lock->lock & 0x80000000)
    {
        lock->s_Owner = NULL;
        asm volatile("stlr wzr, [%0]" :: "r"(&lock->lock) : "memory");
        dsb();
        sev();
    }
    else
    {
        unsigned int lock_value, write_result;

        asm volatile(
                "1:     ldxr    %w0, [%2]       \n\t"   // Read lock value and gain exclusive access
                "       sub     %w0, %w0, #1    \n\t"   // One reader less
                "       stlxr   %w1, %w0, [%2]  \n\t"   // Try to update the lock value
                "       cbnz    %w1, 1b         \n\t"   // Try again if the write failed
                : "=&r"(lock_value), "=&r"(write_result)
                : "r"(&lock->lock)
                : "memory"
        );

        /* Send event to other cores if lock is free */
        if (lock_value == 0)
        {
            dsb();
            sev();
        }
    }

    AROS_LIBFUNC_EXIT
}
