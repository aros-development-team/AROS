/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

AROS_LH1(void, KrnSpinInit,
	AROS_LHA(spinlock_t *, lock, A0),
	struct KernelBase *, KernelBase, 40, Kernel)
{
    AROS_LIBFUNC_INIT

    return;

    AROS_LIBFUNC_EXIT
}
