#    Copyright © 2018, The AROS Development Team. All rights reserved.
#    $Id$
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(signbit881, STDC, 128)
	_FUNCTION(AROS_SLIB_ENTRY(signbit881, STDC, 128))
	
AROS_SLIB_ENTRY(signbit881, STDC, 128):
	moveq	#31,%d1
	asr.l	%d1,%d0
	rts
