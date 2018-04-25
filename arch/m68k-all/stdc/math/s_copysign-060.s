#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(copysign060, STDC, 169)
	_FUNCTION(AROS_SLIB_ENTRY(copysign060, STDC, 169))
	
AROS_SLIB_ENTRY(copysign060, STDC, 169):
	fmove.s	%d0,%fp0
	fabs.x	%fp0
	tst.l	%d1
	bpl	.copysign060done
	fneg.x	%fp0
.copysign060done:
	fmove.s	%fp0,%d0
	rts
