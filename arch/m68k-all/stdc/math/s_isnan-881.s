#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(isnan881, STDC, 111)
	_FUNCTION(AROS_SLIB_ENTRY(isnan881, STDC, 111))
	
AROS_SLIB_ENTRY(isnan881, STDC, 111):
	fmove.s	%d0,%fp0
	ftst.x	%fp0
	moveq	#1,%d0
	fbun	.isnan881done
	moveq	#0,%d0
.isnan881done:
	rts
