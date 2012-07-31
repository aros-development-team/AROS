
	#include "aros/m68k/asm.h"

	.chip 68040
	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Mul_6888x,MathIeeeSingBas,13)
	.type	AROS_SLIB_ENTRY(Mul_6888x,MathIeeeSingBas,13),@function
AROS_SLIB_ENTRY(Mul_6888x,MathIeeeSingBas,13):
	fmove.s %d0,%fp0
	fmul.s %d1,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Div_6888x,MathIeeeSingBas,14)
	.type	AROS_SLIB_ENTRY(Div_6888x,MathIeeeSingBas,14),@function
AROS_SLIB_ENTRY(Div_6888x,MathIeeeSingBas,14):
	fmove.s %d0,%fp0
	fdiv.s %d1,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Add_6888x,MathIeeeSingBas,11)
	.type	AROS_SLIB_ENTRY(Add_6888x,MathIeeeSingBas,11),@function
AROS_SLIB_ENTRY(Add_6888x,MathIeeeSingBas,11):
	fmove.s %d0,%fp0
	fadd.s %d1,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sub_6888x,MathIeeeSingBas,12)
	.type	AROS_SLIB_ENTRY(Sub_6888x,MathIeeeSingBas,12),@function
AROS_SLIB_ENTRY(Sub_6888x,MathIeeeSingBas,12):
	fmove.s %d0,%fp0
	fsub.s %d1,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Neg_6888x,MathIeeeSingBas,10)
	.type	AROS_SLIB_ENTRY(Neg_6888x,MathIeeeSingBas,10),@function
AROS_SLIB_ENTRY(Neg_6888x,MathIeeeSingBas,10):
	fneg.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Abs_6888x,MathIeeeSingBas,9)
	.type	AROS_SLIB_ENTRY(Abs_6888x,MathIeeeSingBas,9),@function
AROS_SLIB_ENTRY(Abs_6888x,MathIeeeSingBas,9):
	fabs.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Fix_6888x,MathIeeeSingBas,5)
	.type	AROS_SLIB_ENTRY(Fix_6888x,MathIeeeSingBas,5),@function
AROS_SLIB_ENTRY(Fix_6888x,MathIeeeSingBas,5):
	fmove.s %d0,%fp0
	fmove.l %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Flt_6888x,MathIeeeSingBas,6)
	.type	AROS_SLIB_ENTRY(Flt_6888x,MathIeeeSingBas,6),@function
AROS_SLIB_ENTRY(Flt_6888x,MathIeeeSingBas,6):
	fmove.l %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.globl init_6888x_single
init_6888x_single:
	/* Round to nearest, single precision */
	moveq #0,%d0
	move.w #0x00+0x40,%d0
	fmove.l %d0,%fpcr
	rts