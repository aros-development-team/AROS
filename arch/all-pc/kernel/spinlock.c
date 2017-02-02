/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <utility/hooks.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH3(spinlock_t *, KrnSpinLock,
	AROS_LHA(spinlock_t *, lock, A1),
	AROS_LHA(struct Hook *, failhook, A0),
	AROS_LHA(ULONG, mode, D0),
	struct KernelBase *, KernelBase, 43, Kernel)
{
    AROS_LIBFUNC_INIT

    if (mode == SPINLOCK_MODE_WRITE)
    {
    }
    else
    {
    }

    return lock;

    AROS_LIBFUNC_EXIT
}
