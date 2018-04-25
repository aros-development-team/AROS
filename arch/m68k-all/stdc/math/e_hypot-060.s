#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(hypot060, STDC, 149)
	_FUNCTION(AROS_SLIB_ENTRY(hypot060, STDC, 149))
	
AROS_SLIB_ENTRY(hypot060, STDC, 149):
	fmove.s	%d0,%fp0
	fmove.s	%d1,%fp1
	fmul.x	%fp0,%fp0
	fmul.x	%fp1,%fp1
	fadd.x	%fp1,%fp0
	fsqrt.x	%fp0
	fmove.s	%fp0,%d0
	rts
