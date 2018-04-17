/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(scalbnf)
	_FUNCTION(AROS_CDEFNAME(scalbnf))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg

	.set	SecondArg, 8 /* Skip FirstArg */
	.set	arg_n, SecondArg

AROS_CDEFNAME(scalbnf):
	fildl	arg_n(%esp)
	flds	arg_x(%esp)
	fscale
	fstp	%st(1)		/* bug fix for fp stack overflow */
	ret

.globl AROS_CDEFNAME(ldexpf)
.set	AROS_CDEFNAME(ldexpf),AROS_CDEFNAME(scalbnf)
