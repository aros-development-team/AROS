/*****************************************************************************

    NAME
#include <proto/kernel.h>

AROS_UFH1(void, FpuRestoreContext,
	AROS_UFHA(struct FpuContext *, fpu, A0))

    SYNOPSIS

    LOCATION

    FUNCTION
	Restore FPU Context

    INPUTS
	None

    RESULT
	None

    NOTES
	Must be called in supervisor mode.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	FpuRestoreContext

FpuRestoreContext:
	fmovem.l	%a0@(0),%fpcr
	fmovem.l	%a0@(4),%fpsr
	fmovem.l	%a0@(8),%fpiar
	fmovem.x	%a0@(3*4),%fp0-%fp7
	frestore	%a0@(3*4 + 8*12)
	rts
