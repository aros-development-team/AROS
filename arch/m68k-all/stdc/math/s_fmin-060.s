#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(fmin060, STDC, 175)
	_FUNCTION(AROS_SLIB_ENTRY(fmin060, STDC, 175))
	
AROS_SLIB_ENTRY(fmin060, STDC, 175):
	fmove.s	%d0,%fp0
	fmove.s	%d1,%fp1
	ftst.x	%fp1
	fbun	.fmin060done
	fcmp.x	%fp1,%fp0
	fbole	.fmin060done
	fmove.x	%fp1,%fp0
.fmin060done:
	fmove.s	%fp0,%d0
	rts
