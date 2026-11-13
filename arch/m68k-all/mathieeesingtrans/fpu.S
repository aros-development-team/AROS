
	#include "aros/m68k/asm.h"

	.chip 68040
	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(ATan_6888x,MathIeeeSingTrans,5)
	.type	AROS_SLIB_ENTRY(ATan_6888x,MathIeeeSingTrans,5),@function
AROS_SLIB_ENTRY(ATan_6888x,MathIeeeSingTrans,5):
	fatan.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sin_6888x,MathIeeeSingTrans,6)
	.type	AROS_SLIB_ENTRY(Sin_6888x,MathIeeeSingTrans,6),@function
AROS_SLIB_ENTRY(Sin_6888x,MathIeeeSingTrans,6):
	fsin.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Cos_6888x,MathIeeeSingTrans,7)
	.type	AROS_SLIB_ENTRY(Cos_6888x,MathIeeeSingTrans,7),@function
AROS_SLIB_ENTRY(Cos_6888x,MathIeeeSingTrans,7):
	fcos.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Tan_6888x,MathIeeeSingTrans,8)
	.type	AROS_SLIB_ENTRY(Tan_6888x,MathIeeeSingTrans,8),@function
AROS_SLIB_ENTRY(Tan_6888x,MathIeeeSingTrans,8):
	ftan.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeSingTrans,9)
	.type	AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeSingTrans,9),@function
AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeSingTrans,9):
	fsincos.s %d0,%fp1,%fp0
	fmove.s %fp1,%a0@
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeSingTrans,10)
	.type	AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeSingTrans,10),@function
AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeSingTrans,10):
	fsinh.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeSingTrans,11)
	.type	AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeSingTrans,11),@function
AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeSingTrans,11):
	fcosh.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeSingTrans,12)
	.type	AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeSingTrans,12),@function
AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeSingTrans,12):
	ftanh.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Exp_6888x,MathIeeeSingTrans,13)
	.type	AROS_SLIB_ENTRY(Exp_6888x,MathIeeeSingTrans,13),@function
AROS_SLIB_ENTRY(Exp_6888x,MathIeeeSingTrans,13):
	fetox.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Log_6888x,MathIeeeSingTrans,14)
	.type	AROS_SLIB_ENTRY(Log_6888x,MathIeeeSingTrans,14),@function
AROS_SLIB_ENTRY(Log_6888x,MathIeeeSingTrans,14):
	flogn.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

/*

Pow does not exist, use software

	.balign 4
	.globl	AROS_SLIB_ENTRY(Pow_6888x,MathIeeeSingTrans,15)
	.type	AROS_SLIB_ENTRY(Pow_6888x,MathIeeeSingTrans,15),@function
AROS_SLIB_ENTRY(Pow_6888x,MathIeeeSingTrans,15):
*/

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeSingTrans,16)
	.type	AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeSingTrans,16),@function
AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeSingTrans,16):
	fsqrt.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

/*
	.balign 4
	.globl	AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeSingTrans,17)
	.type	AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeSingTrans,17),@function
AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeSingTrans,17):
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeSingTrans,18)
	.type	AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeSingTrans,18),@function
AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeSingTrans,18):
	rts
*/

	.balign 4
	.globl	AROS_SLIB_ENTRY(Asin_6888x,MathIeeeSingTrans,19)
	.type	AROS_SLIB_ENTRY(Asin_6888x,MathIeeeSingTrans,19),@function
AROS_SLIB_ENTRY(Asin_6888x,MathIeeeSingTrans,19):
	fasin.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Acos_6888x,MathIeeeSingTrans,20)
	.type	AROS_SLIB_ENTRY(Acos_6888x,MathIeeeSingTrans,20),@function
AROS_SLIB_ENTRY(Acos_6888x,MathIeeeSingTrans,20):
	facos.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Log10_6888x,MathIeeeSingTrans,21)
	.type	AROS_SLIB_ENTRY(Log10_6888x,MathIeeeSingTrans,21),@function
AROS_SLIB_ENTRY(Log10_6888x,MathIeeeSingTrans,21):
	flog10.s %d0,%fp0
	fmove.s %fp0,%d0
	rts
