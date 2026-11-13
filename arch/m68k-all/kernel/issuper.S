/*****************************************************************************

    NAME
#include <proto/kernel.h>

AROS_LH0I(int, KrnIsSuper,

    SYNOPSIS

    LOCATION
	struct KernelBase *, KernelBase, 13, Kernel)

    FUNCTION
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
	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl AROS_SLIB_ENTRY(KrnIsSuper,Kernel,13)
	.globl Kernel_Is_Super

AROS_SLIB_ENTRY(KrnIsSuper,Kernel,13):
Kernel_Is_Super:
	/* 68010+ privilege violation exception handler checks for this special case.
	 * (move from SR is not privileged on 68000) */
	move %sr,%d0
	/* If above instruction caused exception, exception handler returns here */
	and.l	#0x2000,%d0
	rts
