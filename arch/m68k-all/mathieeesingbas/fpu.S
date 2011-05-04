
	#include "aros/m68k/asm.h"

	.chip 68040
	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Mul_6888x,MathIeeeSingBas)
	.type	AROS_SLIB_ENTRY(Mul_6888x,MathIeeeSingBas),@function
AROS_SLIB_ENTRY(Mul_6888x,MathIeeeSingBas):
	fmove.s %d0,%fp0
	fmul.s %d1,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Div_6888x,MathIeeeSingBas)
	.type	AROS_SLIB_ENTRY(Div_6888x,MathIeeeSingBas),@function
AROS_SLIB_ENTRY(Div_6888x,MathIeeeSingBas):
	fmove.s %d0,%fp0
	fdiv.s %d1,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Add_6888x,MathIeeeSingBas)
	.type	AROS_SLIB_ENTRY(Add_6888x,MathIeeeSingBas),@function
AROS_SLIB_ENTRY(Add_6888x,MathIeeeSingBas):
	fmove.s %d0,%fp0
	fadd.s %d1,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sub_6888x,MathIeeeSingBas)
	.type	AROS_SLIB_ENTRY(Sub_6888x,MathIeeeSingBas),@function
AROS_SLIB_ENTRY(Sub_6888x,MathIeeeSingBas):
	fmove.s %d0,%fp0
	fsub.s %d1,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Neg_6888x,MathIeeeSingBas)
	.type	AROS_SLIB_ENTRY(Neg_6888x,MathIeeeSingBas),@function
AROS_SLIB_ENTRY(Neg_6888x,MathIeeeSingBas):
	fneg.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Abs_6888x,MathIeeeSingBas)
	.type	AROS_SLIB_ENTRY(Abs_6888x,MathIeeeSingBas),@function
AROS_SLIB_ENTRY(Abs_6888x,MathIeeeSingBas):
	fabs.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Fix_6888x,MathIeeeSingBas)
	.type	AROS_SLIB_ENTRY(Fix_6888x,MathIeeeSingBas),@function
AROS_SLIB_ENTRY(Fix_6888x,MathIeeeSingBas):
	fmove.s %d0,%fp0
	fmove.l %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Flt_6888x,MathIeeeSingBas)
	.type	AROS_SLIB_ENTRY(Flt_6888x,MathIeeeSingBas),@function
AROS_SLIB_ENTRY(Flt_6888x,MathIeeeSingBas):
    fmove.l %d0,%fp0
    fmove.s %fp0,%d0
	rts