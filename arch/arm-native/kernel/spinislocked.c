/*
    Copyright � 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH1(int, KrnSpinIsLocked,
	AROS_LHA(spinlock_t *, lock, A0),
	struct KernelBase *, KernelBase, 50, Kernel)
{
    AROS_LIBFUNC_INIT

    if (lock->lock == 0)
        return 0;
    else
        return 1;

    AROS_LIBFUNC_EXIT
}
