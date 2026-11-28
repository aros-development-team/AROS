#ifndef KERNEL_SPINLOCK_H_
#define KERNEL_SPINLOCK_H_
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Atomic steps of the kernel spinlock, RISC-V version.
          See rom/kernel/kernel_spinlock.h for what each one must do.

    The lock word follows <aros/types/spinlock_s.h>: 0 is free,
    SPINLOCKF_WRITE (bit 27) is held for writing, and a non-zero
    readcount (bits 0-23) counts readers. Every update is a single
    LR/SC sequence on the whole word, so the SPINLOCKF_UPDATING bit the
    byte-CAS x86 version needs is never set here, but it is honoured as
    "busy" so the layout stays interchangeable.
*/

#include <aros/types/spinlock_s.h>

#include "kernel_riscv.h"

#define SPINLOCK_BUSY_FOR_READ  (SPINLOCKF_WRITE | SPINLOCKF_UPDATING)

/* One attempt at a write lock; TRUE if it was taken */
static inline BOOL krnRVSpinTryWrite(spinlock_t *lock)
{
    unsigned int old, fail;

    asm volatile(
        "1:     lr.w.aq     %0, (%2)        \n"
        "       bnez        %0, 2f          \n"     /* held - give up       */
        "       sc.w        %1, %3, (%2)    \n"
        "       bnez        %1, 1b          \n"     /* lost the reservation */
        "2:                                 \n"
        : "=&r"(old), "=&r"(fail)
        : "r"(&lock->lock), "r"(SPINLOCKF_WRITE)
        : "memory");

    return (old == 0);
}

/* One attempt at a read lock; TRUE if it was taken */
static inline BOOL krnRVSpinTryRead(spinlock_t *lock)
{
    unsigned int old, fail, tmp;

    asm volatile(
        "1:     lr.w.aq     %0, (%3)        \n"
        "       and         %2, %0, %4      \n"
        "       bnez        %2, 2f          \n"     /* write-held - give up */
        "       addiw       %2, %0, 1       \n"
        "       sc.w        %1, %2, (%3)    \n"
        "       bnez        %1, 1b          \n"
        "2:                                 \n"
        : "=&r"(old), "=&r"(fail), "=&r"(tmp)
        : "r"(&lock->lock), "r"(SPINLOCK_BUSY_FOR_READ)
        : "memory");

    return !(old & SPINLOCK_BUSY_FOR_READ);
}

/* Release ordering publishes the critical section first */
static inline void krnRVSpinReleaseWrite(spinlock_t *lock)
{
    asm volatile("amoswap.w.rl zero, zero, (%0)" :: "r"(&lock->lock) : "memory");
}

/* One reader fewer - unless it was not held for reading at all */
static inline void krnRVSpinReleaseRead(spinlock_t *lock)
{
    unsigned int old, fail, tmp;

    asm volatile(
        "1:     lr.w        %0, (%3)        \n"
        "       li          %2, 0xffffff    \n"
        "       and         %2, %0, %2      \n"
        "       beqz        %2, 2f          \n"     /* no readers: leave it */
        "       addiw       %2, %0, -1      \n"
        "       sc.w.rl     %1, %2, (%3)    \n"
        "       bnez        %1, 1b          \n"
        "2:                                 \n"
        : "=&r"(old), "=&r"(fail), "=&r"(tmp)
        : "r"(&lock->lock)
        : "memory");
}

/*
 * The shared KrnSpinLock() body in rom/kernel uses these names. A
 * uniprocessor build has no real spinlocks, as before.
 */
#if defined(__AROSEXEC_SMP__)
#define KRN_SPINLOCK_ATOMICS

#define krnSpinTryWrite(lock)       krnRVSpinTryWrite(lock)
#define krnSpinTryRead(lock)        krnRVSpinTryRead(lock)
#define krnSpinReleaseWrite(lock)   krnRVSpinReleaseWrite(lock)
#define krnSpinReleaseRead(lock)    krnRVSpinReleaseRead(lock)
#define krnSpinPause()              krnRVPause()
#define krnSpinThisTask()           TLS_GET(ThisTask)

#if defined(RISCV_KRNWATCHDOG)
/*
 * Leave a trail for the watchdog (kernel_watchdog.c) while a hart waits
 * for a lock; an interrupt's wait can nest inside this one.
 */
#define krnSpinWaitDecl                                         \
    tls_t *__sw_tls = TLS_PTR_GET();                            \
    APTR __sw_wait = NULL, __sw_caller = NULL;                  \
    UQUAD __sw_since = 0;                                       \
    BOOL __sw_waited = FALSE;

#define krnSpinWaitBegin(lock)                                  \
    do {                                                        \
        if (!__sw_waited)                                       \
        {                                                       \
            __sw_waited = TRUE;                                 \
            __sw_wait = __sw_tls->SpinWait;                     \
            __sw_caller = __sw_tls->SpinCaller;                 \
            __sw_since = __sw_tls->SpinSince;                   \
            __sw_tls->SpinSince = krnRVReadTime();              \
            __sw_tls->SpinCaller = __builtin_return_address(0); \
            __sw_tls->SpinWait = (lock);                        \
        }                                                       \
    } while (0)

#define krnSpinWaitEnd()                                        \
    do {                                                        \
        if (__sw_waited)                                        \
        {                                                       \
            __sw_tls->SpinWait = __sw_wait;                     \
            __sw_tls->SpinCaller = __sw_caller;                 \
            __sw_tls->SpinSince = __sw_since;                   \
        }                                                       \
    } while (0)
#endif /* RISCV_KRNWATCHDOG */
#endif /* __AROSEXEC_SMP__ */

#endif /* KERNEL_SPINLOCK_H_ */
