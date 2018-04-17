/*
 * Based on code written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/x86_64/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(scalbnl)
	_FUNCTION(AROS_CDEFNAME(scalbnl))

	.set	FirstArg, 8 /* Skip Return-Adress */
	.set	arg_x, FirstArg

	.set	SecondArg, 16 /* Skip FirstArg */
	.set	arg_y, SecondArg
	
AROS_CDEFNAME(scalbnl):
	movl	%edi,-4(%rsp)
	fildl	-4(%rsp)
	fldt	8(%rsp)
	fscale
	fstp	%st(1)
	ret

.globl	AROS_CDEFNAME(ldexpl)
.set	AROS_CDEFNAME(ldexpl),AROS_CDEFNAME(scalbnl)
