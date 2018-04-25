#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(floor060)
	_FUNCTION(AROS_CDEFNAME(floor060))
	
AROS_CDEFNAME(floor060):
	fmove.s	%d0,%fp0
	fmove.x	%fp0,%fp1
	fintrz.x	%fp0
	fcmp.x	%fp1,%fp0
	fbole	.floor060done
	fsub.s	#0x3f800000,%fp0
.floor060done:
	fmove.s	%fp0,%d0
	rts
