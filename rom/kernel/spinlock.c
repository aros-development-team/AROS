/*
    Copyright © 2015-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/types/spinlock_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <utility/hooks.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH3(spinlock_t *, KrnSpinLock,

/*  SYNOPSIS */
        AROS_LHA(spinlock_t *, lock, A1),
        AROS_LHA(struct Hook *, failhook, A0),
        AROS_LHA(ULONG, mode, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 52, Kernel)

/*  FUNCTION

    INPUTS
        lock      - spinlock to lock
        failhook  - called if the lock is already held
        mode      - type of lock to obtain.

    RESULT
        returns a pointer to the spinlock "handle".

    NOTES
        The failhook is necessary, because it is not safe
        for code running in user space to spin on a shared lock.
        If a lower priority task holds the lock, it will never be released.
           Because of  this, Exec uses a hook that puts the task on
           a spinning list, to allow other code to run until the lock is
           released.  At this point the "spinning" task will wake and
           obtain the lock.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return lock;

    AROS_LIBFUNC_EXIT
}
