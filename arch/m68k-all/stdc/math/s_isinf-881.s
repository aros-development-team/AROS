#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(isinf881, STDC, 118)
	_FUNCTION(AROS_SLIB_ENTRY(isinf881, STDC, 118))
	
AROS_SLIB_ENTRY(isinf881, STDC, 118):
	fmove.s	%d0,%fp0
	ftst.x	%fp0
	moveq	#1,%d0
	fmove.l	%fpsr,%d1
	rol.l	#7,%d1
	and.l	%d1,%d0
	rts
