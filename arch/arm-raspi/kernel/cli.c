/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

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

    D(bug("[KRN] KrnCli()\n"));

    asm volatile("cpsid i\n");

    AROS_LIBFUNC_EXIT
}
