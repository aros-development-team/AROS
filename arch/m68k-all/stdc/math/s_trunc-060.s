#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(trunc060)
	_FUNCTION(AROS_CDEFNAME(trunc060))
	
AROS_CDEFNAME(trunc060):
	fmove.s	%d0,%fp0
	fintrz.x	%fp0
	fmove.s	%fp0,%d0
	rts
