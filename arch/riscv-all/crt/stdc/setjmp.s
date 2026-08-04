/*
 * Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
 * $Id$
 *
 * ILP32D layout (see aros/stdc/setjmp.h, _JMPLEN 37):
 *   0:      ra
 *   4-48:   s0-s11
 *   52:     sp
 *   56-144: fs0-fs11 (8 bytes each, 8-aligned)
 */

#include "aros/riscv/asm.h"

	.text
	.align	2
	.global AROS_CDEFNAME(setjmp)
	.type	AROS_CDEFNAME(setjmp),%function

AROS_CDEFNAME(setjmp):
	sw	ra, 0(a0)					/* store return address explicitly */
	sw	s0, 1*4(a0)
	sw	s1, 2*4(a0)
	sw	s2, 3*4(a0)
	sw	s3, 4*4(a0)
	sw	s4, 5*4(a0)
	sw	s5, 6*4(a0)
	sw	s6, 7*4(a0)
	sw	s7, 8*4(a0)
	sw	s8, 9*4(a0)
	sw	s9, 10*4(a0)
	sw	s10, 11*4(a0)
	sw	s11, 12*4(a0)
	sw	sp, 13*4(a0)
	fsd	fs0, 56(a0)
	fsd	fs1, 64(a0)
	fsd	fs2, 72(a0)
	fsd	fs3, 80(a0)
	fsd	fs4, 88(a0)
	fsd	fs5, 96(a0)
	fsd	fs6, 104(a0)
	fsd	fs7, 112(a0)
	fsd	fs8, 120(a0)
	fsd	fs9, 128(a0)
	fsd	fs10, 136(a0)
	fsd	fs11, 144(a0)
	li	a0, 0						/* return zero */
	ret
