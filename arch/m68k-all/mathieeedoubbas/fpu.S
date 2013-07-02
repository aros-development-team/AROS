
	#include "aros/m68k/asm.h"

	.chip 68040
	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Mul_6888x,MathIeeeDoubBas, 13)
	.type	AROS_SLIB_ENTRY(Mul_6888x,MathIeeeDoubBas, 13),@function
AROS_SLIB_ENTRY(Mul_6888x,MathIeeeDoubBas, 13):
	movem.l %d0-%d3,%sp@-
	fmove.d %sp@+,%fp0
	fmul.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Div_6888x,MathIeeeDoubBas, 14)
	.type	AROS_SLIB_ENTRY(Div_6888x,MathIeeeDoubBas, 14),@function
AROS_SLIB_ENTRY(Div_6888x,MathIeeeDoubBas, 14):
	movem.l %d0-%d3,%sp@-
	fmove.d %sp@+,%fp0
	fdiv.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Add_6888x,MathIeeeDoubBas, 11)
	.type	AROS_SLIB_ENTRY(Add_6888x,MathIeeeDoubBas, 11),@function
AROS_SLIB_ENTRY(Add_6888x,MathIeeeDoubBas, 11):
	movem.l %d0-%d3,%sp@-
	fmove.d %sp@+,%fp0
	fadd.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sub_6888x,MathIeeeDoubBas, 12)
	.type	AROS_SLIB_ENTRY(Sub_6888x,MathIeeeDoubBas, 12),@function
AROS_SLIB_ENTRY(Sub_6888x,MathIeeeDoubBas, 12):
	movem.l %d0-%d3,%sp@-
	fmove.d %sp@+,%fp0
	fsub.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Neg_6888x,MathIeeeDoubBas, 10)
	.type	AROS_SLIB_ENTRY(Neg_6888x,MathIeeeDoubBas, 10),@function
AROS_SLIB_ENTRY(Neg_6888x,MathIeeeDoubBas, 10):
	movem.l %d0-%d1,%sp@-
	fneg.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Abs_6888x,MathIeeeDoubBas, 9)
	.type	AROS_SLIB_ENTRY(Abs_6888x,MathIeeeDoubBas, 9),@function
AROS_SLIB_ENTRY(Abs_6888x,MathIeeeDoubBas, 9):
	movem.l %d0-%d1,%sp@-
	fabs.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Fix_6888x,MathIeeeDoubBas, 5)
	.type	AROS_SLIB_ENTRY(Fix_6888x,MathIeeeDoubBas, 5),@function
AROS_SLIB_ENTRY(Fix_6888x,MathIeeeDoubBas, 5):
	movem.l %d0-%d1,%sp@-
	fmove.d %sp@+,%fp0
	fmove.l %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Flt_6888x,MathIeeeDoubBas, 6)
	.type	AROS_SLIB_ENTRY(Flt_6888x,MathIeeeDoubBas, 6),@function
AROS_SLIB_ENTRY(Flt_6888x,MathIeeeDoubBas, 6):
	fmove.l %d0,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.globl init_6888x_double
init_6888x_double:
	/* Round to zero, double precision */
	moveq #0,%d0
	move.w #0x10+0x80,%d0
	fmove.l %d0,%fpcr
	rts