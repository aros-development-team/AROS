#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(copysign060)
	_FUNCTION(AROS_CDEFNAME(copysign060))
	
AROS_CDEFNAME(copysign060):
	fmove.s	%d0,%fp0
	fabs.x	%fp0
	tst.l	%d1
	bpl	.copysign060done
	fneg.x	%fp0
.copysign060done:
	fmove.s	%fp0,%d0
	rts
