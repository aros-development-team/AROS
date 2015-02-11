/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function siglongjmp()
    Lang: english
*/

/******************************************************************************

    NAME
#include <setjmp.h>

	void siglongjmp (jmp_buf env, int val);

    FUNCTION
	Save the current context so that you can return to it later.

    INPUTS
	env - The context/environment to restore
	val - This value is returned by setjmp() when you return to the
		saved context. You cannot return 0. If val is 0, then
		setjmp() returns with 1.

    RESULT
	This function doesn't return.

    NOTES

    EXAMPLE
	jmp_buf env;

	... some code ...

	if (!setjmp (env))
	{
	    ... this code is executed after setjmp() returns ...

	    // This is no good example on how to use this function
	    // You should not do that
	    if (error)
		siglongjmp (env, 5);

	    ... some code ...
	}
	else
	{
	    ... this code is executed if you call siglongjmp(env) ...
	}

    BUGS

    SEE ALSO
	setjmp()

    INTERNALS

    HISTORY

******************************************************************************/

	#include "aros/i386/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(siglongjmp)
	_FUNCTION(AROS_CDEFNAME(siglongjmp))

	.set	FirstArg, 4 /* Skip Return-Adress */
	.set	env, FirstArg
	.set	val, env+4

AROS_CDEFNAME(siglongjmp):
	/* Fetch the address of the env-structure off the stack.
	    The address is stored in %eax which is not preserved
	    because it's contents are overwritten anyway by the
	    return code */
	movl env(%esp),%eax

	/* Read return value into %ebx and make sure it's not 0 */
	movl val(%esp),%ebx
	cmpl $0,%ebx
	jne  1f

	movl $1,%ebx
1:
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
