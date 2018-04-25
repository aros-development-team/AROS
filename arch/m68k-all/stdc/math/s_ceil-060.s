#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(ceil060)
	_FUNCTION(AROS_CDEFNAME(ceil060))
	
AROS_CDEFNAME(ceil060):
	fmove.s	%d0,%fp0
	fmove.x	%fp0,%fp1
	fintrz.x	%fp0
	fcmp.x	%fp1,%fp0
	fboge	.ceil060done
	fadd.s	#0x3f800000,%fp0
.ceil060done:
	fmove.s	%fp0,%d0
	rts
