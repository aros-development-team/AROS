/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <utility/hooks.h>

#include <asm/cpu.h>

#include <kernel_base.h>

#include "tls.h"

#include <proto/kernel.h>

/*
 * Lock word: 0 free, bit 31 write-held, anything else a reader count.
 * Waiters park in wfe and are woken by the sev in KrnSpinUnLock; the sevl
 * primes the event register so a free lock is never waited on.
 */
AROS_LH3(spinlock_t *, KrnSpinLock,
        AROS_LHA(spinlock_t *, lock, A1),
        AROS_LHA(struct Hook *, failhook, A0),
        AROS_LHA(ULONG, mode, D0),
        struct KernelBase *, KernelBase, 52, Kernel)
{
    AROS_LIBFUNC_INIT

    unsigned int lock_value, result;

    if (mode == SPINLOCK_MODE_WRITE)
    {
        asm volatile(
                "       sevl                    \n\t"
                "1:     wfe                     \n\t"
                "2:     ldaxr   %w0, [%2]       \n\t"   // Load the lock value, gaining exclusive access
                "       cbnz    %w0, 1b         \n\t"   // Taken? Wait for the unlock event
                "       stxr    %w1, %w3, [%2]  \n\t"   // Try to write the write-locked value
                "       cbnz    %w1, 2b         \n\t"   // Exclusive access lost - re-examine the lock
                : "=&r"(lock_value), "=&r"(result)
                : "r"(&lock->lock), "r"(0x80000000)
                : "memory"
        );

        /* Without an owner a leaked lock is just a set bit. */
        lock->s_Owner = TLS_GET(ThisTask);
    }
    else
    {
        asm volatile(
                "       sevl                    \n\t"
                "1:     wfe                     \n\t"
                "2:     ldaxr   %w0, [%2]       \n\t"   // Load the lock value, gaining exclusive access
                "       tbnz    %w0, #31, 1b    \n\t"   // Write-held? Wait for the unlock event
                "       add     %w0, %w0, #1    \n\t"   // One more reader
                "       stxr    %w1, %w0, [%2]  \n\t"   // Try to write the new reader count
                "       cbnz    %w1, 2b         \n\t"   // Exclusive access lost - re-examine the lock
                : "=&r"(lock_value), "=&r"(result)
                : "r"(&lock->lock)
                : "memory"
        );
    }

    return lock;

    AROS_LIBFUNC_EXIT
}
