#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(fmax881, STDC, 174)
	_FUNCTION(AROS_SLIB_ENTRY(fmax881, STDC, 174))
	
AROS_SLIB_ENTRY(fmax881, STDC, 174):
	fmove.s	%d0,%fp0
	fmove.s	%d1,%fp1
	ftst.x	%fp1
	fbun	.fmax881done
	fcmp.x	%fp1,%fp0
	fboge	.fmax881done
	fmove.x	%fp1,%fp0
.fmax881done:
	fmove.s	%fp0,%d0
	rts
