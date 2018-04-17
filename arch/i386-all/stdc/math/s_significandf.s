/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(significandf)
	_FUNCTION(AROS_CDEFNAME(significandf))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg

AROS_CDEFNAME(significandf):
	flds	arg_x(%esp)
	fxtract
	fstp	%st(1)
	ret
