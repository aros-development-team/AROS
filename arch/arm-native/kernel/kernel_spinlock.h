#ifndef KERNEL_SPINLOCK_H_
#define KERNEL_SPINLOCK_H_
/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc: Atomic steps of the kernel spinlock, ARM version.
          See rom/kernel/kernel_spinlock.h for what each one must do.

    Every update is one exclusive load and store on the whole word. A CPU
    that has to wait parks in wfe; the releasing CPU wakes it with sev. An
    event that arrives between a failed attempt and the wfe stays latched,
    so the wfe returns at once and no wake-up is lost.
*/

#include <asm/arm/cpu.h>
#include <aros/types/spinlock_s.h>

#define KRN_SPINLOCK_ATOMICS

/* The task running on this CPU */
#include "tls.h"
#define krnSpinThisTask()       TLS_GET(ThisTask)

#define SPINLOCK_BUSY_FOR_READ  (SPINLOCKF_WRITE | SPINLOCKF_UPDATING)

static inline BOOL krnSpinTryWrite(spinlock_t *lock)
{
    unsigned long lock_value, tmp;

    do
    {
        asm volatile(
                ".syntax unified                \n\t"
                "       ldrex   %0, [%2]        \n\t"   // Load the lock value and gain exclusive access
                "       teq     %0, #0          \n\t"   // Is the lock free?
                "       strexeq %1, %3, [%2]    \n\t"   // Store the write-locked value and check the result
                : "=&r"(lock_value), "=&r"(tmp)
                : "r"(&lock->lock), "r"(SPINLOCKF_WRITE), "1"(0)
                : "cc", "memory"
        );
    } while (tmp);      /* Retry only when exclusive access was lost */

    if (lock_value != 0)
        return FALSE;

    /* Acquire fence: pairs with the dmb in the release functions */
    dmb();
    return TRUE;
}

static inline BOOL krnSpinTryRead(spinlock_t *lock)
{
    unsigned long lock_value, newval, tmp;

    do
    {
        asm volatile(
                ".syntax unified                \n\t"
                "       ldrex   %0, [%3]        \n\t"   // Load the lock value and gain exclusive access
                "       tst     %0, %4          \n\t"   // Held for writing?
                "       addeq   %1, %0, #1      \n\t"   // No: one more reader
                "       strexeq %2, %1, [%3]    \n\t"   // Store the new count and check the result
                : "=&r"(lock_value), "=&r"(newval), "=&r"(tmp)
                : "r"(&lock->lock), "r"(SPINLOCK_BUSY_FOR_READ), "2"(0)
                : "cc", "memory"
        );
    } while (tmp);

    if (lock_value & SPINLOCK_BUSY_FOR_READ)
        return FALSE;

    dmb();
    return TRUE;
}

/*
 * Release fence: prior critical-section stores must be globally observable
 * before the lock-clear store, since ARMv7-A allows the release store to be
 * reordered ahead of earlier stores. The dsb before the sev keeps the event
 * from overtaking the clear and leaving a wfe-parked waiter asleep.
 */
static inline void krnSpinReleaseWrite(spinlock_t *lock)
{
    dmb();
    lock->lock = SPINLOCK_UNLOCKED;
    dsb();
    sev();
}

static inline void krnSpinReleaseRead(spinlock_t *lock)
{
    unsigned long lock_value, newval, tmp;

    dmb();
    do
    {
        asm volatile(
                ".syntax unified                \n\t"
                "       ldrex   %0, [%3]        \n\t"   // Read lock value and gain exclusive access
                "       bics    %1, %0, #0xff000000 \n\t" // Any reader at all?
                "       subne   %1, %0, #1      \n\t"   // Yes: one reader less
                "       strexne %2, %1, [%3]    \n\t"   // Try to update the lock value
                : "=&r"(lock_value), "=&r"(newval), "=&r"(tmp)
                : "r"(&lock->lock), "2"(0)
                : "cc", "memory"
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
