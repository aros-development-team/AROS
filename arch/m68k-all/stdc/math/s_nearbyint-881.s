#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(nearbyint881, STDC, 158)
	_FUNCTION(AROS_SLIB_ENTRY(nearbyint881, STDC, 158))
	
AROS_SLIB_ENTRY(nearbyint881, STDC, 158):
	fmove.s	%d0,%fp0
	fint.x	%fp0
	fmove.s	%fp0,%d0
	rts
