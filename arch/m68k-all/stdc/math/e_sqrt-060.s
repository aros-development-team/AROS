#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(sqrt060)
	_FUNCTION(AROS_CDEFNAME(sqrt060))
	
AROS_CDEFNAME(sqrt060):
	fmove.s	%d0,%fp0
	fsqrt.x	%fp0
	fmove.s	%fp0,%d0
	rts
