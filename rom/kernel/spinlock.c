/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(spinlock_t *, KrnSpinLock,

/*  SYNOPSIS */
	AROS_LHA(spinlock_t *, lock, A0),
	AROS_LHA(ULONG, mode, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 38, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return lock;

    AROS_LIBFUNC_EXIT
}
