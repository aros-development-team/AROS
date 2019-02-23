/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <asm/arm/cpu.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH2(spinlock_t *, KrnSpinTryLock,
	AROS_LHA(spinlock_t *, lock, A0),
	AROS_LHA(ULONG, mode, D0),
	struct KernelBase *, KernelBase, 51, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned long lock_value, tmp;

    if (mode == SPINLOCK_MODE_WRITE)
    {
        do
        {
            asm volatile(
                    "       ldrex   %0, [%2]        \n\t"   // Load the lock value and gain exclusive lock to memory
                    "       teq     %0, #0          \n\t"   // is the value 0? It means the lock was free
                    "       strexeq %1, %3, [%2]    \n\t"   // Store value to memory and check if it succeeded
                    : "=&r"(lock_value), "=&r"(tmp)
                    : "r" (&lock->lock), "r"(0x80000000), "1"(0)
                    : "cc"
            );
        } while(tmp);

        if (lock_value == 0)
        {
            dmb();
            return lock;
        }
        else
            return NULL;
    }
    else
    {
        do
        {
            asm volatile(
                    "       ldrex   %0, [%2]        \n\t"   // Load lock value and gain exclusive lock to memory
                    "       adds    %0, %0, #1      \n\t"   // Increase the lock value and update conditional bits
                    "       strexpl %1, %0, [%2]    \n\t"   // Store value to memory if positive and check result
                    : "=&r"(lock_value), "=&r"(tmp)
                    : "r"(&lock->lock), "1"(0)
                    : "cc"
            );
        } while(tmp);

        if (lock_value < 0x80000000)
        {
            dmb();
            return lock;
        }
        else
            return NULL;
    }

    AROS_LIBFUNC_EXIT
}
