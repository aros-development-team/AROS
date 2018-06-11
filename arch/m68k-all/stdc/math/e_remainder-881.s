#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(remainder881, STDC, 177)
	_FUNCTION(AROS_SLIB_ENTRY(remainder881, STDC, 177))
	
AROS_SLIB_ENTRY(remainder881, STDC, 177):
	fmove.s	%d0,%fp0
	fmove.s	%d1,%fp1
	frem.x	%fp1,%fp0
	fmove.s	%fp0,%d0
	rts
