#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(fma060)
	_FUNCTION(AROS_CDEFNAME(fma060))
	
AROS_CDEFNAME(fma060):
	fmove.s	%d0,%fp0
	fmove.s	%d1,%fp1
	fmove.s	%d2,%fp2
	fmul.x	%fp1,%fp0
	fadd.x	%fp2,%fp0
	fmove.s	%fp0,%d0
	rts
