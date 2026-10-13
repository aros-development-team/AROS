/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <asm/cpu.h>

#include <kernel_base.h>

#include "tls.h"

#include <proto/kernel.h>

/* Single attempt: clrex and fail rather than park. tmp stays 0 when the
 * store is skipped, so we retry only on lost exclusive access. */
AROS_LH2(spinlock_t *, KrnSpinTryLock,
        AROS_LHA(spinlock_t *, lock, A0),
        AROS_LHA(ULONG, mode, D0),
        struct KernelBase *, KernelBase, 51, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned int lock_value, tmp;

    if (mode == SPINLOCK_MODE_WRITE)
    {
        do
        {
            asm volatile(
                    "       ldaxr   %w0, [%2]       \n\t"   // Load the lock value, gaining exclusive access
                    "       cbnz    %w0, 1f         \n\t"   // Taken - give up without spinning
                    "       stxr    %w1, %w3, [%2]  \n\t"   // Try to write the write-locked value
                    "       b       2f              \n\t"
                    "1:     clrex                   \n\t"   // Drop the dangling exclusive monitor
                    "2:                             \n\t"
                    : "=&r"(lock_value), "=&r"(tmp)
                    : "r"(&lock->lock), "r"(0x80000000), "1"(0)
                    : "memory"
            );
        } while(tmp);

        if (lock_value == 0)
        {
            lock->s_Owner = TLS_GET(ThisTask);
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
                    "       ldaxr   %w0, [%2]       \n\t"   // Load the lock value, gaining exclusive access
                    "       tbnz    %w0, #31, 1f    \n\t"   // Write-held - give up without spinning
                    "       add     %w0, %w0, #1    \n\t"   // One more reader
                    "       stxr    %w1, %w0, [%2]  \n\t"   // Try to write the new reader count
                    "       b       2f              \n\t"
                    "1:     clrex                   \n\t"   // Drop the dangling exclusive monitor
                    "2:                             \n\t"
                    : "=&r"(lock_value), "=&r"(tmp)
                    : "r"(&lock->lock), "1"(0)
                    : "memory"
            );
        } while(tmp);

        if (lock_value < 0x80000000)
            return lock;
        else
            return NULL;
    }

    AROS_LIBFUNC_EXIT
}
