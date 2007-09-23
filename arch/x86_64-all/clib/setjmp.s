/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: setjmp.s 12741 2001-12-08 18:16:08Z chodorowski $

    Desc: ANSI C function setjmp()
    Lang: english
*/

/******************************************************************************

    NAME
#include <setjmp.h>

	int setjmp (jmp_buf env);

    FUNCTION
	Save the current context so that you can return to it later.

    INPUTS
	env - The context/environment is saved here for later restoring

    RESULT
	0 if it returns from setjmp() and 1 when it returns from longjmp().

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
	longjmp()

    INTERNALS

    HISTORY

******************************************************************************/

	#include "aros/x86_64/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(setjmp)
	_FUNCTION(AROS_CDEFNAME(setjmp))

	.set	retaddr, 0

AROS_CDEFNAME(setjmp):
	/* Save stack pointer and all registers into env */
	mov %rbx,8(%rdi) /* %ebx */
	mov %rcx,16(%rdi) /* %ecx */
	mov %rdx,24(%rdi) /* %edx */
	mov %rsi,32(%rdi) /* %esi */
	mov %rdi,40(%rdi) /* %edi */
	mov %rbp,48(%rdi) /* %ebp */
	mov %r8,56(%rdi)
	mov %r9,64(%rdi)
	mov %r10,72(%rdi)
	mov %r11,80(%rdi)
	mov %r12,88(%rdi)
	mov %r13,96(%rdi)
	mov %r14,104(%rdi)
	mov %r15,112(%rdi)
	mov %rsp,120(%rdi) /* %esp */

	mov retaddr(%rsp),%rax /* Save return address (%esp has changed) */
	mov %rax,0(%rdi)

	xor %rax,%rax /* Return 0 */
	ret
