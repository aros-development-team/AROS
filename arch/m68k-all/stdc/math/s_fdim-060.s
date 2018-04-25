#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(fdim060)
	_FUNCTION(AROS_CDEFNAME(fdim060))
	
AROS_CDEFNAME(fdim060):
	fmove.s	%d0,%fp0
	fmove.s	%d1,%fp1
	fsub.x	%fp1,%fp0
	fabs.x	%fp0
	fmove.s	%fp0,%d0
	rts
