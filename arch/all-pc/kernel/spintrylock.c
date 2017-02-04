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

#define D(x)

AROS_LH2(spinlock_t *, KrnSpinTryLock,
	AROS_LHA(spinlock_t *, lock, A0),
	AROS_LHA(ULONG, mode, D0),
	struct KernelBase *, KernelBase, 42, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(0x%p, %08x)\n", __func__, lock, mode));

    if (((mode == SPINLOCK_MODE_WRITE) && (lock->lock != 0)) ||
        (lock->slock.write))
            return NULL;

#if (1) // defined(AROS_NO_ATOMIC_OPERATIONS)
    lock->slock.readcount++;
#else
    AROS_ATOMIC_INC(lock->slock.readcount);
#endif

    return lock;

    AROS_LIBFUNC_EXIT
}
