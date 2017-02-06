/*
    Copyright � 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

#define D(x)

AROS_LH1(void, KrnSpinInit,
	AROS_LHA(spinlock_t *, lock, A0),
	struct KernelBase *, KernelBase, 40, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(0x%p)\n", __func__, lock));

    lock->lock = SPINLOCK_UNLOCKED;

    return;

    AROS_LIBFUNC_EXIT
}
