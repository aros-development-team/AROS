#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(round060)
	_FUNCTION(AROS_CDEFNAME(round060))
	
AROS_CDEFNAME(round060):
	fmove.s	%d0,%fp0
	and.l	#0x80000000,%d0
	or.l	#0x3f000000,%d0
	fadd.s	%d0,%fp0
	fintrz.x	%fp0
	fmove.s	%fp0,%d0
	rts
