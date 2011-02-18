/*****************************************************************************

    NAME
#include <proto/kernel.h>

AROS_UFH2(void, FpuRestoreContext,
	AROS_UFHA(struct FpuContext *, fpu, A0)
	AROS_UFHA(UWORD, nulloffset, D0))

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
	tst.b		3*4 + 8*12(%a0,%d0.w)
	beq.s		0f
	fmovem.l	%a0@,%fpcr/%fpsr/%fpiar
	fmovem.x	%a0@(3*4),%fp0-%fp7
0:	frestore	%a0@(3*4 + 8*12)
	rts
