
	#include "aros/m68k/asm.h"

	.chip 68040
	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Mul_6888x,MathIeeeDoubBas)
	.type	AROS_SLIB_ENTRY(Mul_6888x,MathIeeeDoubBas),@function
AROS_SLIB_ENTRY(Mul_6888x,MathIeeeDoubBas):
	movem.l %d0-%d3,%sp@-
	fmove.d %sp@+,%fp0
	fmul.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Div_6888x,MathIeeeDoubBas)
	.type	AROS_SLIB_ENTRY(Div_6888x,MathIeeeDoubBas),@function
AROS_SLIB_ENTRY(Div_6888x,MathIeeeDoubBas):
	movem.l %d0-%d3,%sp@-
	fmove.d %sp@+,%fp0
	fdiv.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Add_6888x,MathIeeeDoubBas)
	.type	AROS_SLIB_ENTRY(Add_6888x,MathIeeeDoubBas),@function
AROS_SLIB_ENTRY(Add_6888x,MathIeeeDoubBas):
	movem.l %d0-%d3,%sp@-
	fmove.d %sp@+,%fp0
	fadd.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sub_6888x,MathIeeeDoubBas)
	.type	AROS_SLIB_ENTRY(Sub_6888x,MathIeeeDoubBas),@function
AROS_SLIB_ENTRY(Sub_6888x,MathIeeeDoubBas):
	movem.l %d0-%d3,%sp@-
	fmove.d %sp@+,%fp0
	fsub.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Neg_6888x,MathIeeeDoubBas)
	.type	AROS_SLIB_ENTRY(Neg_6888x,MathIeeeDoubBas),@function
AROS_SLIB_ENTRY(Neg_6888x,MathIeeeDoubBas):
	movem.l %d0-%d1,%sp@-
	fneg.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Abs_6888x,MathIeeeDoubBas)
	.type	AROS_SLIB_ENTRY(Abs_6888x,MathIeeeDoubBas),@function
AROS_SLIB_ENTRY(Abs_6888x,MathIeeeDoubBas):
	movem.l %d0-%d1,%sp@-
	fabs.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Fix_6888x,MathIeeeDoubBas)
	.type	AROS_SLIB_ENTRY(Fix_6888x,MathIeeeDoubBas),@function
AROS_SLIB_ENTRY(Fix_6888x,MathIeeeDoubBas):
	movem.l %d0-%d1,%sp@-
	fmove.d %sp@+,%fp0
	fmove.l %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Flt_6888x,MathIeeeDoubBas)
	.type	AROS_SLIB_ENTRY(Flt_6888x,MathIeeeDoubBas),@function
AROS_SLIB_ENTRY(Flt_6888x,MathIeeeDoubBas):
	fmove.l %d0,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts