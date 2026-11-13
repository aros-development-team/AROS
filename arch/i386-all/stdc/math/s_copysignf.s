/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(copysignf)
	_FUNCTION(AROS_CDEFNAME(copysignf))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg

	.set	SecondArg, 8 /* Skip FirstArg */
	.set	arg_y, SecondArg
	
AROS_CDEFNAME(copysignf):
	movl	arg_y(%esp),%edx
	andl	$0x80000000,%edx
	movl	arg_x(%esp),%eax
	andl	$0x7fffffff,%eax
	orl	%edx,%eax
	movl	%eax,arg_x(%esp)
	flds	arg_x(%esp)

	ret
