#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(asin881, STDC, 131)
	_FUNCTION(AROS_SLIB_ENTRY(asin881, STDC, 131))
	
AROS_SLIB_ENTRY(asin881, STDC, 131):
	fmove.s	%d0,%fp0
	fasin.x	%fp0
	fmove.s	%fp0,%d0
	rts
