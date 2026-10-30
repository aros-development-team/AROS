#ifndef KERNEL_SPINLOCK_H_
#define KERNEL_SPINLOCK_H_
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Spinlock primitives for native RISC-V SMP kernels.

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

/*
 * The owner, for debugging a leaked lock: the task that took it, or
 * NULL when it was taken in trap context (as on all-pc).
 */
static inline void *krnRVSpinOwner(void)
{
    return (KRN_TRAPDEPTH > 0) ? NULL : (void *)TLS_GET(ThisTask);
}

#endif /* KERNEL_SPINLOCK_H_ */
