/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH1(void, KrnSpinUnLock,
	AROS_LHA(spinlock_t *, lock, A0),
	struct KernelBase *, KernelBase, 44, Kernel)
{
    AROS_LIBFUNC_INIT

    if (lock->lock == 0)
        return;

    // TODO: release the lock.

    AROS_LIBFUNC_EXIT
}
