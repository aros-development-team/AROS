/*
 * Based on code written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(copysignl)
	_FUNCTION(AROS_CDEFNAME(copysignl))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	arg_x, FirstArg
	.set	FirstArg_lo, 12 /* Skip Return-Adress */
	.set	arg_x_lo, FirstArg_lo

	.set	SecondArg, 24 /* Skip FirstArg */
	.set	arg_y, SecondArg
	
AROS_CDEFNAME(copysignl):
	movl	arg_y(%esp),%edx
	andl	$0x8000,%edx
	movl	arg_x_lo(%esp),%eax
	andl	$0x7fff,%eax
	orl	%edx,%eax
	movl	%eax,arg_x_lo(%esp)
	fldt	arg_x(%esp)

	ret
