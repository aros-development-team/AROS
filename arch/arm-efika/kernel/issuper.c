/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/arm/cpucontext.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0I(int, KrnIsSuper,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 13, Kernel)

/*  FUNCTION
	Determine if the caller is running in supervisor mode

    INPUTS
	None

    RESULT
	Nonzero for supervisor mode, zero for user mode

    NOTES
	Callers should only test the return value against zero.
	Nonzero values may actually be different, since they
	may carry some private implementation-dependent information
	(like CPU privilege level, for example).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    register unsigned int reg;
    asm volatile("mrs %[reg], cpsr" : [reg] "=r" (reg) );

    return !(((reg & CPUMODE_MASK) == CPUMODE_USER) || ((reg & CPUMODE_MASK) == CPUMODE_SYSTEM));

    AROS_LIBFUNC_EXIT
}
