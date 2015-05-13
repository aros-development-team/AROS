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
	struct KernelBase *, KernelBase, 43, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned long lock_value, result;

    if (mode == SPINLOCK_MODE_WRITE)
    {
        asm volatile(
                "1:     ldrex   %0, [%1]        \n\t"   // Load the lock value and gain exclusive lock to memory
                "       teq     %0, #0          \n\t"   // is the value 0? It means the lock was free
                "       wfene                   \n\t"   // Wait for Event if lock was not free
                "       strexeq %0, %2, [%1]    \n\t"   // Store value to memory and check if it succeeded
                "       teq     %0, #0          \n\t"   // Test if write succeeded
                "       bne     1b              \n\t"   // Try again if locking failed
                : "=&r"(lock_value)
                : "r" (&lock->lock), "r"(0x80000000)
                : "cc"
        );
    }
    else
    {
        asm volatile(
                "1:     ldrex   %0, [%2]        \n\t"   // Load lock value and gain exclusive lock to memory
                "       adds    %0, %0, #1      \n\t"   // Increase the lock value and update conditional bits
                "       wfemi                   \n\t"   // Wait for event if lock value was negative
                "       strexpl %1, %0, [%2]    \n\t"   // Store value to memory if positive and check result
                "       rsbpls  %0, %1, #0      \n\t"   // Reverse substract and update conditionals: if strex write was
                                                        // successful, %0 contains 0. #0 - %0 clears N flag. If write failed
                                                        // %0 contains 1. #0 - %0 sets N flag (value 0xffffffff).
                "       bmi     1b              \n\t"   // Try again if N flag is set (because of locok value or write failure
                : "=&r"(lock_value), "=&r"(result)
                : "r"(&lock->lock)
                : "cc"
        );
    }

    dmb();
    return lock;

    AROS_LIBFUNC_EXIT
}
