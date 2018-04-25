#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(isnormal060)
	_FUNCTION(AROS_CDEFNAME(isnormal060))
	
AROS_CDEFNAME(isnormal060):
	fmove.s	%d0,%fp0
	ftst.x	%fp0
	fmove.l	%fpsr,%d0
	and.l	#0x07000000,%d0
	seq	%d0
	extb.l	%d0
	rts
