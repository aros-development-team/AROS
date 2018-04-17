/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(log10f)
	_FUNCTION(AROS_CDEFNAME(log10f))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg

AROS_CDEFNAME(log10f):
	fldlg2
	flds	arg_x(%esp)
	fyl2x
	ret
