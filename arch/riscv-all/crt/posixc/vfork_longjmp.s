/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: vfork_longjmp() - like longjmp but without the val check
    Lang: english

    Returns via the retaddr slot - __vfork() patches it to redirect the
    jump (this is the whole point of vfork_longjmp).

    Layout (see aros/stdc/setjmp.h); SZREG is 4 on RV32 (ILP32D, _JMPLEN 37),
    8 on RV64 (LP64D, _JMPLEN 25):
      0:                ra (retaddr)
      1*SZREG-12*SZREG: s0-s11
      13*SZREG:         sp
      14*SZREG + 8*k:   fs0-fs11 (fs<k>, 8 bytes each, 8-aligned)
*/

#include "aros/riscv/xlen.h"

	.text
	.align	2
	.global	AROS_CDEFNAME(vfork_longjmp)
	.type	AROS_CDEFNAME(vfork_longjmp),%function

AROS_CDEFNAME(vfork_longjmp):
	/* a0 = pointer to jmp_buf, a1 = return value */
	REG_L	ra, 0(a0)					/* return via the retaddr slot */
	REG_L	s0, 1*SZREG(a0)
	REG_L	s1, 2*SZREG(a0)
	REG_L	s2, 3*SZREG(a0)
	REG_L	s3, 4*SZREG(a0)
	REG_L	s4, 5*SZREG(a0)
	REG_L	s5, 6*SZREG(a0)
	REG_L	s6, 7*SZREG(a0)
	REG_L	s7, 8*SZREG(a0)
	REG_L	s8, 9*SZREG(a0)
	REG_L	s9, 10*SZREG(a0)
	REG_L	s10, 11*SZREG(a0)
	REG_L	s11, 12*SZREG(a0)
	REG_L	sp, 13*SZREG(a0)
	fld	fs0, 14*SZREG+0*8(a0)
	fld	fs1, 14*SZREG+1*8(a0)
	fld	fs2, 14*SZREG+2*8(a0)
	fld	fs3, 14*SZREG+3*8(a0)
	fld	fs4, 14*SZREG+4*8(a0)
	fld	fs5, 14*SZREG+5*8(a0)
	fld	fs6, 14*SZREG+6*8(a0)
	fld	fs7, 14*SZREG+7*8(a0)
	fld	fs8, 14*SZREG+8*8(a0)
	fld	fs9, 14*SZREG+9*8(a0)
	fld	fs10, 14*SZREG+10*8(a0)
	fld	fs11, 14*SZREG+11*8(a0)

	/* No zero-check on val (unlike longjmp) */
	mv	a0, a1
	ret
