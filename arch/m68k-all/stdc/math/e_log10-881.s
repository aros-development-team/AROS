#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(log10881, STDC, 150)
	_FUNCTION(AROS_SLIB_ENTRY(log10881, STDC, 150))
	
AROS_SLIB_ENTRY(log10881, STDC, 150):
	fmove.s	%d0,%fp0
	flog10.x	%fp0
	fmove.s	%fp0,%d0
	rts
