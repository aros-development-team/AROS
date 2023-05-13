/*
    Copyright © 2023, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function siglongjmp()
    Lang: english
*/

#include "aros/riscv/asm.h"

	.text
	.align	2
	.global	AROS_CDEFNAME(siglongjmp)
	.type	AROS_CDEFNAME(siglongjmp),%function

AROS_CDEFNAME(siglongjmp):
	lw	ra, 0(a0)
	lw	s0, 1*4(a0)
	lw	s1, 2*4(a0)
	lw	s2, 3*4(a0)
	lw	s3, 4*4(a0)
	lw	s4, 5*4(a0)
	lw	s5, 6*4(a0)
	lw	s6, 7*4(a0)
	lw	s7, 8*4(a0)
	lw	s8, 9*4(a0)
	lw	s9, 10*4(a0)
	lw	s10, 11*4(a0)
	lw	s11, 12*4(a0)

	beq a1, zero, slongjmp_1
	mv a0, a1
	ret

slongjmp_1:
	li a0, 1
	ret