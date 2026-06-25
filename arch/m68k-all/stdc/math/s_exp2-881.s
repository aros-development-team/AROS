#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(exp2881, STDC, 144)
	_FUNCTION(AROS_SLIB_ENTRY(exp2881, STDC, 144))
	
AROS_SLIB_ENTRY(exp2881, STDC, 144):
	fmove.s	%d0,%fp0
	ftwotox.x	%fp0
	fmove.s	%fp0,%d0
	rts
