/*
 * Based on the i387 version written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/x86_64/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(fmodf)
	_FUNCTION(AROS_CDEFNAME(fmodf))

	.set	FirstArg, 8 /* Skip Return-Adress */
	.set	arg_x, FirstArg

	.set	SecondArg, 16 /* Skip FirstArg */
	.set	arg_y, SecondArg
	
AROS_CDEFNAME(fmodf):
	movss	%xmm0,-4(%rsp)
	movss	%xmm1,-8(%rsp)
	flds	-8(%rsp)
	flds	-4(%rsp)
1:	fprem
	fstsw	%ax
	testw	$0x400,%ax
	jne	1b
	fstps	-4(%rsp)
	movss	-4(%rsp),%xmm0
	fstp	%st
	ret
