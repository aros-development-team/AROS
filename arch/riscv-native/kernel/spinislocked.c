/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Report whether a spinlock is held.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

#include <kernel_base.h>

#include "kernel_spinlock.h"

#include <proto/kernel.h>

/* See rom/kernel/spinislocked.c for documentation */

AROS_LH1(int, KrnSpinIsLocked,
        AROS_LHA(spinlock_t *, lock, A0),
        struct KernelBase *, KernelBase, 50, Kernel)
{
    AROS_LIBFUNC_INIT

    return (lock->lock != SPINLOCK_UNLOCKED) ? 1 : 0;

    AROS_LIBFUNC_EXIT
}
