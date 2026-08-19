/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <asm/arm/cpu.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH1(void, KrnSpinUnLock,
        AROS_LHA(spinlock_t *, lock, A0),
        struct KernelBase *, KernelBase, 53, Kernel)
{
    AROS_LIBFUNC_INIT

    if (lock->lock == 0)
        return;

    /*
     * Release fence: prior critical-section stores must be globally
     * observable before the lock-clear store. Without this dmb, a
     * subsequent acquirer on another core can take the lock and read
     * stale data because ARMv7-A allows the lock-release store to be
     * reordered ahead of earlier stores. Pairs with the acquire dmb
     * in KrnSpinLock.
     */
    dmb();

    /*
     * Are we releasing a write lock? Just zero out the value. Also send event so that other cores waiting for lock
     * wake up.
     */
    if (lock->lock & 0x80000000)
    {
        lock->lock = 0;
        /*
         * dsb before sev: the event must not overtake the lock-clear
         * store, or a wfe-parked waiter wakes, re-reads the still-locked
         * value and parks again (recovery then depends on the global
         * monitor generating another event).
         */
        dsb();
        asm volatile("sev");
    }
    else
    {
        unsigned long lock_value, write_result;

        asm volatile(
                "1:     ldrex   %0, [%2]        \n\t"   // Read lock value and gain write exclusive lock
                "       sub     %0, %0, #1      \n\t"   // Decrease lock value
                "       strex   %1, %0, [%2]    \n\t"   // Try to update the lock value
                "       teq     %1, #0          \n\t"   // test if write succeeded
                "       bne     1b              \n\t"   // Try again if write failed
                :"=&r"(lock_value), "=&r"(write_result)
                :"r"(&lock->lock)
                :"cc"
        );

        /* Send event to other cores if lock is free */
        if (lock_value == 0)
        {
            dsb();
            asm volatile("sev");
        }
    }

    AROS_LIBFUNC_EXIT
}
