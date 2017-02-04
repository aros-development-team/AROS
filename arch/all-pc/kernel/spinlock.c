/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <utility/hooks.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

#define D(x)

AROS_LH3(spinlock_t *, KrnSpinLock,
	AROS_LHA(spinlock_t *, lock, A1),
	AROS_LHA(struct Hook *, failhook, A0),
	AROS_LHA(ULONG, mode, D0),
	struct KernelBase *, KernelBase, 43, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(0x%p, 0x%p, %08x)\n", __func__, lock, failhook, mode));

    if (mode == SPINLOCK_MODE_WRITE)
    {
        while (lock->lock != 0) 
        {
            if (failhook)
            {
                D(bug("[Kernel] %s: lock-held ... calling fail hook...\n", __func__));
//                CALLHOOKPKT(failhook, (APTR)lock, 0);
            }
        };
        lock->slock.write = 1;
    }
    else
    {
        while (lock->slock.write) 
        {
            if (failhook)
            {
                D(bug("[Kernel] %s: write-locked .. calling fail hook...\n", __func__));
//                CALLHOOKPKT(failhook, (APTR)lock, 0);
            }
        };
    }

#if (1) // defined(AROS_NO_ATOMIC_OPERATIONS)
    lock->slock.readcount++;
#else
    AROS_ATOMIC_INC(lock->slock.readcount);
#endif

    return lock;

    AROS_LIBFUNC_EXIT
}
