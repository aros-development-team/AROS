/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Release a spinlock.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

#include <kernel_base.h>

#include "kernel_spinlock.h"

#include <proto/kernel.h>

/* See rom/kernel/spinunlock.c for documentation */

AROS_LH1(void, KrnSpinUnLock,
        AROS_LHA(spinlock_t *, lock, A0),
        struct KernelBase *, KernelBase, 53, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    unsigned int old, fail, tmp;

    if (lock->lock & SPINLOCKF_WRITE)
    {
        lock->s_Owner = NULL;
        /* Release ordering publishes the critical section first */
        asm volatile("amoswap.w.rl zero, zero, (%0)" :: "r"(&lock->lock) : "memory");
        return;
    }

    /* One reader fewer - unless it was not held at all */
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
#else
    (void)lock;
#endif

    AROS_LIBFUNC_EXIT
}
