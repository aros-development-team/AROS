#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(lround881, STDC, 163)
	_FUNCTION(AROS_SLIB_ENTRY(lround881, STDC, 163))
	
AROS_SLIB_ENTRY(lround881, STDC, 163):
	fmove.l	%d0,%fp0
	and.l	#0x80000000,%d0
	or.l	#0x3f000000,%d0
	fadd.s	%d0,%fp0
	fintrz.x	%fp0
	fmove.l	%fp0,%d0
	rts
