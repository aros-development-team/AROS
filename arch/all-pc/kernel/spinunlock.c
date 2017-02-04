/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

#define D(x) x

AROS_LH1(void, KrnSpinUnLock,
	AROS_LHA(spinlock_t *, lock, A0),
	struct KernelBase *, KernelBase, 44, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(0x%p)\n", __func__, lock));

    if (lock->slock.write)
        lock->slock.write = 0;

#if (1) // defined(AROS_NO_ATOMIC_OPERATIONS)
    lock->slock.readcount--;
#else
    AROS_ATOMIC_DEC(lock->slock.readcount);
#endif

    return;

    AROS_LIBFUNC_EXIT
}
