/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
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
	struct KernelBase *, KernelBase, 43, Kernel)

/*  FUNCTION

    INPUTS
	lock      - spinlock to lock
	failhook  - called if the lock is already held
	mode      - type of lock to obtain.

    RESULT
	returns a pointer to the spinlock "handle".

    NOTES

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
