#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(fma881, STDC, 186)
	_FUNCTION(AROS_SLIB_ENTRY(fma881, STDC, 186))
	
AROS_SLIB_ENTRY(fma881, STDC, 186):
	fmove.s	%d0,%fp0
	fmove.s	%d1,%fp1
	fmove.s	%d2,%fp2
	fmul.x	%fp1,%fp0
	fadd.x	%fp2,%fp0
	fmove.s	%fp0,%d0
	rts
