#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(cos881, STDC, 124)
	_FUNCTION(AROS_SLIB_ENTRY(cos881, STDC, 124))
	
AROS_SLIB_ENTRY(cos881, STDC, 124):
	fmove.s	%d0,%fp0
	fcos.x	%fp0
	fmove.s	%fp0,%d0
	rts
