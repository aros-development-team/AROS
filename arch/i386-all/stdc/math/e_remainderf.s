/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(remainderf)
	_FUNCTION(AROS_CDEFNAME(remainderf))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg

	.set	SecondArg, 8 /* Skip FirstArg */
	.set	arg_y, SecondArg
	
AROS_CDEFNAME(remainderf):
	flds	arg_y(%esp)
	flds	arg_x(%esp)
1:	fprem1
	fstsw	%ax
	sahf
	jp	1b
	fstp	%st(1)
	ret
