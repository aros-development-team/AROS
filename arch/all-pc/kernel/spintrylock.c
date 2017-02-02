/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH2(spinlock_t *, KrnSpinTryLock,
	AROS_LHA(spinlock_t *, lock, A0),
	AROS_LHA(ULONG, mode, D0),
	struct KernelBase *, KernelBase, 42, Kernel)
{
    AROS_LIBFUNC_INIT

    if (mode == SPINLOCK_MODE_WRITE)
    {
    }
    else
    {
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}
