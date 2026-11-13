/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(logbf)
	_FUNCTION(AROS_CDEFNAME(logbf))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg
	
AROS_CDEFNAME(logbf):
	flds	arg_x(%esp)
	fxtract
	fstp	%st
	ret
