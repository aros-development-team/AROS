/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 function siglongjmp()
    Lang: english

    ILP32D layout (see aros/stdc/setjmp.h, _JMPLEN 37):
      0:      ra (retaddr)
      4-48:   s0-s11
      52:     sp
      56-144: fs0-fs11 (8 bytes each, 8-aligned)
*/

#include "aros/riscv/asm.h"

	.text
	.align	2
	.global	AROS_CDEFNAME(siglongjmp)
	.type	AROS_CDEFNAME(siglongjmp),%function

AROS_CDEFNAME(siglongjmp):
	/* a0 = pointer to jmp_buf, a1 = return value */
	lw	ra, 0(a0)					/* return via the retaddr slot */
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
	lw	sp, 13*4(a0)
	fld	fs0, 56(a0)
	fld	fs1, 64(a0)
	fld	fs2, 72(a0)
	fld	fs3, 80(a0)
	fld	fs4, 88(a0)
	fld	fs5, 96(a0)
	fld	fs6, 104(a0)
	fld	fs7, 112(a0)
	fld	fs8, 120(a0)
	fld	fs9, 128(a0)
	fld	fs10, 136(a0)
	fld	fs11, 144(a0)

	/* return val, or 1 if val was 0 */
	bne	a1, zero, 1f
	li	a1, 1
1:
	mv	a0, a1
	ret
