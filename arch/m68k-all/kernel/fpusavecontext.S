/*****************************************************************************

    NAME
#include <proto/kernel.h>

AROS_UFH1(void, FpuSaveContext,
	AROS_UFHA(struct FpuContext *, fpu, A0))

    SYNOPSIS

    LOCATION

    FUNCTION
	Save FPU Context

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
	.globl	FpuSaveContext

FpuSaveContext:
	fsave		%a0@(3*4 + 8*12)
	fmovem.x	%fp0-%fp7,%a0@(3*4)
	fmovem.l	%fpiar,%a0@(8)
	fmovem.l	%fpsr,%a0@(4)
	fmovem.l	%fpcr,%a0@(0)
	rts
