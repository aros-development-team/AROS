/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(sqrtf)
	_FUNCTION(AROS_CDEFNAME(sqrtf))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg

AROS_CDEFNAME(sqrtf):
	/* Fetch the arguments off the stack. */
	flds	arg_x(%esp)

	fsqrt

	ret
