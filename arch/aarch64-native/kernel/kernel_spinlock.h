#ifndef KERNEL_SPINLOCK_H_
#define KERNEL_SPINLOCK_H_
/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc: Atomic steps of the kernel spinlock, AArch64 version.
          See rom/kernel/kernel_spinlock.h for what each one must do.

    Every update is one exclusive load and store on the whole word. A CPU
    that has to wait parks in wfe; the releasing CPU wakes it with sev. An
    event that arrives between a failed attempt and the wfe stays latched,
    so the wfe returns at once and no wake-up is lost.
*/

#include <asm/cpu.h>
#include <aros/types/spinlock_s.h>

#define KRN_SPINLOCK_ATOMICS

/* The task running on this CPU */
#include "tls.h"
#define krnSpinThisTask()       TLS_GET(ThisTask)

#define SPINLOCK_BUSY_FOR_READ  (SPINLOCKF_WRITE | SPINLOCKF_UPDATING)

static inline BOOL krnSpinTryWrite(spinlock_t *lock)
{
    unsigned int lock_value, tmp;

    do
    {
        asm volatile(
                "       ldaxr   %w0, [%2]       \n\t"   // Load the lock value, gaining exclusive access
                "       cbnz    %w0, 1f         \n\t"   // Taken - give up
                "       stxr    %w1, %w3, [%2]  \n\t"   // Try to write the write-locked value
                "       b       2f              \n\t"
                "1:     clrex                   \n\t"   // Drop the dangling exclusive monitor
                "2:                             \n\t"
                : "=&r"(lock_value), "=&r"(tmp)
                : "r"(&lock->lock), "r"(SPINLOCKF_WRITE), "1"(0)
                : "memory"
        );
    } while (tmp);      /* Retry only when exclusive access was lost */

    return (lock_value == 0);
}

static inline BOOL krnSpinTryRead(spinlock_t *lock)
{
    unsigned int lock_value, newval, tmp;

    do
    {
        asm volatile(
                "       ldaxr   %w0, [%3]       \n\t"   // Load the lock value, gaining exclusive access
                "       tst     %w0, %w4        \n\t"   // Held for writing?
                "       b.ne    1f              \n\t"   // Give up
                "       add     %w1, %w0, #1    \n\t"   // One more reader
                "       stxr    %w2, %w1, [%3]  \n\t"   // Try to write the new reader count
                "       b       2f              \n\t"
                "1:     clrex                   \n\t"   // Drop the dangling exclusive monitor
                "2:                             \n\t"
                : "=&r"(lock_value), "=&r"(newval), "=&r"(tmp)
                : "r"(&lock->lock), "r"(SPINLOCK_BUSY_FOR_READ), "2"(0)
                : "memory", "cc"
        );
    } while (tmp);

    return !(lock_value & SPINLOCK_BUSY_FOR_READ);
}

/*
 * stlr publishes the critical section before the clear; the dsb keeps the
 * sev from overtaking it and leaving a waiter parked.
 */
static inline void krnSpinReleaseWrite(spinlock_t *lock)
{
    asm volatile("stlr wzr, [%0]" :: "r"(&lock->lock) : "memory");
    dsb();
    sev();
}

static inline void krnSpinReleaseRead(spinlock_t *lock)
{
    unsigned int lock_value, newval, tmp;

    do
    {
        asm volatile(
                "       ldxr    %w0, [%3]       \n\t"   // Read lock value and gain exclusive access
                "       tst     %w0, #0xffffff  \n\t"   // Any reader at all?
                "       b.eq    1f              \n\t"   // No: leave it alone
                "       sub     %w1, %w0, #1    \n\t"   // One reader less
                "       stlxr   %w2, %w1, [%3]  \n\t"   // Try to update the lock value
                "       b       2f              \n\t"
                "1:     clrex                   \n\t"
                "2:                             \n\t"
                : "=&r"(lock_value), "=&r"(newval), "=&r"(tmp)
                : "r"(&lock->lock), "2"(0)
                : "memory", "cc"
        );
    } while (tmp);

    /* Wake the waiters when the last reader has left */
    if ((lock_value & 0x00ffffff) == 1)
    {
        dsb();
        sev();
    }
}

static inline void krnSpinPause(void)
{
    asm volatile("wfe" ::: "memory");
}

#endif /* KERNEL_SPINLOCK_H_ */
