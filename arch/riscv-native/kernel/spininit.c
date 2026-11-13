/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Initialise a spinlock.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

#include <kernel_base.h>

#include "kernel_spinlock.h"

#include <proto/kernel.h>

/* See rom/kernel/spininit.c for documentation */

AROS_LH1(void, KrnSpinInit,
        AROS_LHA(spinlock_t *, lock, A0),
        struct KernelBase *, KernelBase, 49, Kernel)
{
    AROS_LIBFUNC_INIT

    lock->lock = SPINLOCK_UNLOCKED;
    lock->s_Owner = NULL;

    AROS_LIBFUNC_EXIT
}
