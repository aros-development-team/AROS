#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(fpclassify060, STDC, 103)
	_FUNCTION(AROS_SLIB_ENTRY(fpclassify060, STDC, 103))
	
AROS_SLIB_ENTRY(fpclassify060, STDC, 103):
	fmove.s	%d0,%fp0
	ftst.x	%fp0
	moveq	#7,%d1
	fmove.l	%fpsr,%d0
	rol.l	#8,%d0
	and.l	%d1,%d0
	rts
