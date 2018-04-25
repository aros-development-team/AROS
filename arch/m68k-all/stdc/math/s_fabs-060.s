#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(fabs060)
	_FUNCTION(AROS_CDEFNAME(fabs060))
	
AROS_CDEFNAME(fabs060):
	fmove.s	%d0,%fp0
	fabs.x	%fp0
	fmove.s	%fp0,%d0
	rts
