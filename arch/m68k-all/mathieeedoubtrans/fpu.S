
	#include "aros/m68k/asm.h"

	.chip 68040
	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(ATan_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(ATan_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(ATan_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fatan.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sin_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Sin_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Sin_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fsin.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Cos_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Cos_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Cos_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fcos.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Tan_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Tan_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Tan_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	ftan.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Sincos_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fsincos.d %sp@+,%fp1,%fp0
	fmove.d %fp1,%a0@
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Sinh_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fsinh.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Cosh_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fcosh.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Tanh_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	ftanh.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Exp_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Exp_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Exp_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fetox.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Log_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Log_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Log_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	flogn.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

/*

Pow does not exist, use software

	.balign 4
	.globl	AROS_SLIB_ENTRY(Pow_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Pow_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Pow_6888x,MathIeeeDoubTrans):
*/

	.balign 4
	.globl	AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Sqrt_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fsqrt.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Tieee_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fmove.d %sp@+,%fp0
	fmove.s %fp0,%d0
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Fieee_6888x,MathIeeeDoubTrans):
	fmove.s %d0,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Asin_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Asin_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Asin_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	fasin.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Acos_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Acos_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Acos_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	facos.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts

	.balign 4
	.globl	AROS_SLIB_ENTRY(Log10_6888x,MathIeeeDoubTrans)
	.type	AROS_SLIB_ENTRY(Log10_6888x,MathIeeeDoubTrans),@function
AROS_SLIB_ENTRY(Log10_6888x,MathIeeeDoubTrans):
	movem.l %d0-%d1,%sp@-
	flog10.d %sp@+,%fp0
	fmove.d %fp0,%sp@-
	movem.l %sp@+,%d0-%d1
	rts
