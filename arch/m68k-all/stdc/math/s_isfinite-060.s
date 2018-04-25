#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(isfinite060)
	_FUNCTION(AROS_CDEFNAME(isfinite060))
	
AROS_CDEFNAME(isfinite060):
	fmove.s	%d0,%fp0
	ftst.x	%fp0
	fmove.l	%fpsr,%d0
	and.l	#0x03000000,%d0
	seq	%d0
	extb.l	%d0
	rts
