/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(fmodf)
	_FUNCTION(AROS_CDEFNAME(fmodf))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg

	.set	SecondArg, 8 /* Skip FirstArg */
	.set	arg_y, SecondArg
	
AROS_CDEFNAME(fmodf):
	flds	arg_y(%esp)
	flds	arg_x(%esp)
1:	fprem
	fstsw	%ax
	sahf
	jp	1b
	fstp	%st(1)
	ret
