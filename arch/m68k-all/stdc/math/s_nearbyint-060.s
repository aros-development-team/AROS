#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(nearbyint060)
	_FUNCTION(AROS_CDEFNAME(nearbyint060))
	
AROS_CDEFNAME(nearbyint060):
	fmove.s	%d0,%fp0
	fint.x	%fp0
	fmove.s	%fp0,%d0
	rts
