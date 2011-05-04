
	#include "aros/m68k/asm.h"

	.chip 68040
	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(ATan_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(ATan_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(ATan_6888x,MathIeeeSingTrans):
	fatan.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sin_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Sin_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Sin_6888x,MathIeeeSingTrans):
	fsin.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Cos_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Cos_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Cos_6888x,MathIeeeSingTrans):
	fcos.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Tan_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Tan_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Tan_6888x,MathIeeeSingTrans):
	ftan.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeSingTrans):
	fsincos.s %d0,%fp1,%fp0
	fmove.s %fp1,%a0@
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeSingTrans):
	fsinh.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeSingTrans):
	fcosh.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeSingTrans):
	ftanh.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Exp_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Exp_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Exp_6888x,MathIeeeSingTrans):
	fetox.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Log_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Log_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Log_6888x,MathIeeeSingTrans):
	flogn.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

/*

Pow does not exist, use software

	.balign 4
	.globl	AROS_SLIB_ENTRY(Pow_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Pow_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Pow_6888x,MathIeeeSingTrans):
*/

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeSingTrans):
	fsqrt.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

/*
	.balign 4
	.globl	AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeSingTrans):
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeSingTrans):
	rts
*/

	.balign 4
	.globl	AROS_SLIB_ENTRY(Asin_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Asin_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Asin_6888x,MathIeeeSingTrans):
	fasin.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Acos_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Acos_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Acos_6888x,MathIeeeSingTrans):
	facos.s %d0,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Log10_6888x,MathIeeeSingTrans)
	.type	AROS_SLIB_ENTRY(Log10_6888x,MathIeeeSingTrans),@function
AROS_SLIB_ENTRY(Log10_6888x,MathIeeeSingTrans):
	flog10.s %d0,%fp0
	fmove.s %fp0,%d0
	rts
