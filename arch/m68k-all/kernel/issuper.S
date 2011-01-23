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
	.globl	AROS_SLIB_ENTRY(KrnIsSuper,Kernel)

AROS_SLIB_ENTRY(KrnIsSuper,Kernel):
	movem.l	%a5-%a6,%sp@-
	movem.l	(4),%a6
	lea	1f,%a5
	jsr	%a6@(Supervisor)
	movem.l	%sp@+,%a5-%a6
	rts

1:	move.w	%sp@,%d0
	and.l	#0x2000,%d0
	rte

