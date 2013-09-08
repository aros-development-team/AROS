/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

/* This function works the same as longjmp() except it lacks the argument 
   check. It's used only by vfork() implementation. */

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(vfork_longjmp)
	_FUNCTION(AROS_CDEFNAME(vfork_longjmp))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	env, FirstArg
	.set	val, env+4

AROS_CDEFNAME(vfork_longjmp):
	/* Fetch the address of the env-structure off the stack.
	    The address is stored in %eax which is not preserved
	    because it's contents are overwritten anyway by the
	    return code */
	movl env(%esp),%eax
    movl val(%esp),%ebx

	/* Restore stack pointer and all registers from env */
	movl 28(%eax),%esp /* Restore original stack */

	movl 0(%eax),%ecx
	movl %ecx,retaddr(%esp) /* Restore return address */

	pushl %ebx /* Save return value on new stack */

	/* Restore all registers */
	movl 4(%eax),%ebx /* %ebx */
	movl 8(%eax),%ecx /* %ecx */
	movl 12(%eax),%edx /* %edx */
	movl 16(%eax),%esi /* %esi */
	movl 20(%eax),%edi /* %edi */
	movl 24(%eax),%ebp /* %ebp */

	popl %eax /* Fetch return value */
	ret
