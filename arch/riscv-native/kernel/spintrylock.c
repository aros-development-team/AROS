/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Try to take a spinlock without waiting.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

#include <kernel_base.h>

#include "kernel_spinlock.h"

#include <proto/kernel.h>

/* See rom/kernel/spintrylock.c for documentation */

AROS_LH2(spinlock_t *, KrnSpinTryLock,
        AROS_LHA(spinlock_t *, lock, A0),
        AROS_LHA(ULONG, mode, D0),
        struct KernelBase *, KernelBase, 51, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    if (mode == SPINLOCK_MODE_WRITE)
    {
        if (!krnRVSpinTryWrite(lock))
            return NULL;
        lock->s_Owner = krnRVSpinOwner();
    }
    else if (!krnRVSpinTryRead(lock))
        return NULL;
#else
    (void)mode;
#endif

    return lock;

    AROS_LIBFUNC_EXIT
}
