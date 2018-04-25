#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(isnan060)
	_FUNCTION(AROS_CDEFNAME(isnan060))
	
AROS_CDEFNAME(isnan060):
	fmove.s	%d0,%fp0
	ftst.x	%fp0
	moveq	#1,%d0
	fbun	.isnan060done
	moveq	#0,%d0
.isnan060done:
	rts
