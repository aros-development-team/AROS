#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(isfinite881, STDC, 105)
	_FUNCTION(AROS_SLIB_ENTRY(isfinite881, STDC, 105))
	
AROS_SLIB_ENTRY(isfinite881, STDC, 105):
	fmove.s	%d0,%fp0
	ftst.x	%fp0
	fmove.l	%fpsr,%d0
	and.l	#0x03000000,%d0
	seq	%d0
	extb.l	%d0
	rts
