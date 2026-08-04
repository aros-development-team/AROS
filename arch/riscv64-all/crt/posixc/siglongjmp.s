/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 function siglongjmp() for 64bit RISC-V
    Lang: english

    LP64D layout (see aros/stdc/setjmp.h, _JMPLEN 25):
      0:       ra (retaddr)
      8-96:    s0-s11
      104:     sp
      112-200: fs0-fs11
*/

#include "aros/riscv64/asm.h"

	.text
	.align	2
	.global	AROS_CDEFNAME(siglongjmp)
	.type	AROS_CDEFNAME(siglongjmp),%function

AROS_CDEFNAME(siglongjmp):
	/* a0 = pointer to jmp_buf, a1 = return value */
	ld	ra, 0(a0)					/* return via the retaddr slot */
	ld	s0, 1*8(a0)
	ld	s1, 2*8(a0)
	ld	s2, 3*8(a0)
	ld	s3, 4*8(a0)
	ld	s4, 5*8(a0)
	ld	s5, 6*8(a0)
	ld	s6, 7*8(a0)
	ld	s7, 8*8(a0)
	ld	s8, 9*8(a0)
	ld	s9, 10*8(a0)
	ld	s10, 11*8(a0)
	ld	s11, 12*8(a0)
	ld	sp, 13*8(a0)
	fld	fs0, 14*8(a0)
	fld	fs1, 15*8(a0)
	fld	fs2, 16*8(a0)
	fld	fs3, 17*8(a0)
	fld	fs4, 18*8(a0)
	fld	fs5, 19*8(a0)
	fld	fs6, 20*8(a0)
	fld	fs7, 21*8(a0)
	fld	fs8, 22*8(a0)
	fld	fs9, 23*8(a0)
	fld	fs10, 24*8(a0)
	fld	fs11, 25*8(a0)

	/* return val, or 1 if val was 0 */
	bne	a1, zero, 1f
	li	a1, 1
1:
	mv	a0, a1
	ret
