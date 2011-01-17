/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function longjmp()
    Lang: english
*/

/******************************************************************************

    NAME
#include <setjmp.h>

	void longjmp (jmp_buf env, int val);

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
		longjmp (env, 5);

	    ... some code ...
	}
	else
	{
	    ... this code is executed if you call longjmp(env) ...
	}

    BUGS

    SEE ALSO
	setjmp()

    INTERNALS

    HISTORY

******************************************************************************/

	#include "aros/x86_64/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(longjmp)
	_FUNCTION(AROS_CDEFNAME(longjmp))

	.set	FirstArg, 8 /* Skip Return-Adress */
	.set	env, FirstArg
	.set	val, env+8
	.set	retaddr, 0

AROS_CDEFNAME(longjmp):

    mov %rdi, %rax
	/* Make sure return value is not 0 */
	cmp $0,%rsi
	jne  1f

	mov $1,%rsi
1:
	/* Restore stack pointer and all registers from env */
	mov 120(%rax),%rsp /* Restore original stack */

	mov 0(%rax),%rcx
	mov %rcx,retaddr(%rsp) /* Restore return address */

	push %rsi /* Save return value on new stack */

	/* Restore all registers */
	mov 8(%rax),%rbx /* %ebx */
	mov 16(%rax),%rcx /* %ecx */
	mov 24(%rax),%rdx /* %edx */
	mov 32(%rax),%rsi /* %esi */
	mov 40(%rax),%rdi /* %edi */
	mov 48(%rax),%rbp /* %ebp */
	mov 56(%rax),%r8
	mov 64(%rax),%r9
	mov 72(%rax),%r10
	mov 80(%rax),%r11
	mov 88(%rax),%r12
	mov 96(%rax),%r13
	mov 104(%rax),%r14
	mov 112(%rax),%r15

	pop %rax /* Fetch return value */
	ret
