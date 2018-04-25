#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(isinf060)
	_FUNCTION(AROS_CDEFNAME(isinf060))
	
AROS_CDEFNAME(isinf060):
	fmove.s	%d0,%fp0
	ftst.x	%fp0
	moveq	#1,%d0
	fmove.l	%fpsr,%d1
	rol.l	#7,%d1
	and.l	%d1,%d0
	rts
