#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(hypot060)
	_FUNCTION(AROS_CDEFNAME(hypot060))
	
AROS_CDEFNAME(hypot060):
	fmove.s	%d0,%fp0
	fmove.s	%d1,%fp1
	fmul.x	%fp0,%fp0
	fmul.x	%fp1,%fp1
	fadd.x	%fp1,%fp0
	fsqrt.x	%fp0
	fmove.s	%fp0,%d0
	rts
