/*
 * Copyright © 2023, The AROS Development Team. All rights reserved.
 * $Id$
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
	li	a0, 0						/* return zero */
	ret
