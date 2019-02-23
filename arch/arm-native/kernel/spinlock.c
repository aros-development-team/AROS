/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <utility/hooks.h>

#include <asm/arm/cpu.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH3(spinlock_t *, KrnSpinLock,
	AROS_LHA(spinlock_t *, lock, A1),
	AROS_LHA(struct Hook *, failhook, A0),
	AROS_LHA(ULONG, mode, D0),
	struct KernelBase *, KernelBase, 52, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned long lock_value, result;

    if (mode == SPINLOCK_MODE_WRITE)
    {
        asm volatile(
                "1:     ldrex   %0, [%1]        \n\t"   // Load the lock value, gaining exclusive access
                "       teq     %0, #0          \n\t"   // Is the lock free?
                "       wfene                   \n\t"   // Wait for Event if allready obtained
                "       strexeq %0, %2, [%1]    \n\t"   // Try to exclusively write the lock value to memory
                "       teq     %0, #0          \n\t"   // Did it succeeded?
                "       bne     1b              \n\t"   // If we failed, try to obtain the lock again
                : "=&r"(lock_value)
                : "r" (&lock->lock), "r"(0x80000000)
                : "cc"
        );
    }
    else
    {
        asm volatile(
                "1:     ldrex   %0, [%2]        \n\t"   // Load the lock value, gaining exclusive access
                "       adds    %0, %0, #1      \n\t"   // Increase the lock value and update conditional bits
                "       wfemi                   \n\t"   // Wait for event if lock value is negative
                "       strexpl %1, %0, [%2]    \n\t"   // Try to exclusively write the lock value to memory if positive
                "       rsbpls  %0, %1, #0      \n\t"   // Reverse substract and update conditionals:
                                                        // - if strex write was successful, %0 contains 0. #0 - %0 clears N flag.
                                                        // - If write failed %0 contains 1. #0 - %0 sets N flag (value 0xffffffff).
                "       bmi     1b              \n\t"   // Try again if N flag is set (because of lock value, or write failure)
                : "=&r"(lock_value), "=&r"(result)
                : "r"(&lock->lock)
                : "cc"
        );
    }

    dmb();
    return lock;

    AROS_LIBFUNC_EXIT
}
