#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(fpclassify060)
	_FUNCTION(AROS_CDEFNAME(fpclassify060))
	
AROS_CDEFNAME(fpclassify060):
	fmove.s	%d0,%fp0
	ftst.x	%fp0
	moveq	#7,%d1
	fmove.l	%fpsr,%d0
	rol.l	#8,%d0
	and.l	%d1,%d0
	rts
