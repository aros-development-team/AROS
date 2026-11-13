/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(scalbnl)
	_FUNCTION(AROS_CDEFNAME(scalbnl))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg

	.set	SecondArg, 16 /* Skip FirstArg */
	.set	arg_n, SecondArg
	
AROS_CDEFNAME(scalbnl):
	fildl	arg_n(%esp)
	fldt	arg_x(%esp)
	fscale
	fstp	%st(1)
	ret

.globl AROS_CDEFNAME(ldexpl)
.set	AROS_CDEFNAME(ldexpl),AROS_CDEFNAME(scalbnl)
