/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Take a spinlock, waiting while it is held.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

#include <kernel_base.h>

#include "kernel_spinlock.h"

#include <proto/kernel.h>

/* See rom/kernel/spinlock.c for documentation */

AROS_LH3(spinlock_t *, KrnSpinLock,
        AROS_LHA(spinlock_t *, lock, A1),
        AROS_LHA(struct Hook *, failhook, A0),
        AROS_LHA(ULONG, mode, D0),
        struct KernelBase *, KernelBase, 52, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * failhook is not called: the waiter just spins, as on the ARM
     * ports (the autodoc allows either).
     */
    (void)failhook;

#if defined(__AROSEXEC_SMP__)
    if (mode == SPINLOCK_MODE_WRITE)
    {
        while (!krnRVSpinTryWrite(lock))
        {
            /* Wait with plain loads, off the reservation */
            while (lock->lock != SPINLOCK_UNLOCKED)
                krnRVPause();
        }
        lock->s_Owner = krnRVSpinOwner();
    }
    else
    {
        while (!krnRVSpinTryRead(lock))
        {
            while (lock->lock & SPINLOCK_BUSY_FOR_READ)
                krnRVPause();
        }
    }
#else
    (void)mode;
#endif

    return lock;

    AROS_LIBFUNC_EXIT
}
