/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(void, KrnCli,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 9, Kernel)

/*  FUNCTION
	Instantly disable interrupts.

    INPUTS
	None

    RESULT
	None

    NOTES
	This is low level function, it does not have nesting count
	and state tracking mechanism. It operates directly on the CPU.
	Normal applications should consider using exec.library/Disable().

    EXAMPLE

    BUGS

    SEE ALSO
	KrnSti()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    krnSysCall(SC_CLI);

    AROS_LIBFUNC_EXIT
}
