/*
 * Copyright (c) 2026, The AROS Development Team. All rights reserved.
 * $Id$
 *
 * LP64D layout (see aros/stdc/setjmp.h, _JMPLEN 25):
 *   0:       ra (retaddr)
 *   8-96:    s0-s11
 *   104:     sp
 *   112-200: fs0-fs11
 */

#include "aros/riscv64/asm.h"

	.text
	.align	2
	.global AROS_CDEFNAME(setjmp)
	.type	AROS_CDEFNAME(setjmp),%function

AROS_CDEFNAME(setjmp):
	sd	ra, 0(a0)					/* store return address explicitly */
	sd	s0, 1*8(a0)
	sd	s1, 2*8(a0)
	sd	s2, 3*8(a0)
	sd	s3, 4*8(a0)
	sd	s4, 5*8(a0)
	sd	s5, 6*8(a0)
	sd	s6, 7*8(a0)
	sd	s7, 8*8(a0)
	sd	s8, 9*8(a0)
	sd	s9, 10*8(a0)
	sd	s10, 11*8(a0)
	sd	s11, 12*8(a0)
	sd	sp, 13*8(a0)
	fsd	fs0, 14*8(a0)
	fsd	fs1, 15*8(a0)
	fsd	fs2, 16*8(a0)
	fsd	fs3, 17*8(a0)
	fsd	fs4, 18*8(a0)
	fsd	fs5, 19*8(a0)
	fsd	fs6, 20*8(a0)
	fsd	fs7, 21*8(a0)
	fsd	fs8, 22*8(a0)
	fsd	fs9, 23*8(a0)
	fsd	fs10, 24*8(a0)
	fsd	fs11, 25*8(a0)
	li	a0, 0						/* return zero */
	ret
