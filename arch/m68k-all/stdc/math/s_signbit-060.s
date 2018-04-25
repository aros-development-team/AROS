#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_CDEFNAME(signbit060)
	_FUNCTION(AROS_CDEFNAME(signbit060))
	
AROS_CDEFNAME(signbit060):
	moveq	#31,%d1
	asr.l	%d1,%d0
	rts
