/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

	#include "machine.i"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(setjmp)
	_FUNCTION(AROS_CDEFNAME(setjmp))

	.set	FirstArg, 8 /* Skip Return-Adress */
	.set	env, FirstArg
	.set	retaddr, 0

AROS_CDEFNAME(setjmp):
	/* Fetch the address of the env-structure off the stack.
	    The address is stored in %eax which is not preserved
	    because it's contents are overwritten anyway by the
	    return code */
	mov env(%rsp),%rax

	/* Save stack pointer and all registers into env */
	mov %rbx,8(%rax) /* %ebx */
	mov %rcx,16(%rax) /* %ecx */
	mov %rdx,24(%rax) /* %edx */
	mov %rsi,32(%rax) /* %esi */
	mov %rdi,40(%rax) /* %edi */
	mov %rbp,48(%rax) /* %ebp */
	mov %rsp,56(%rax) /* %esp */
	mov %r8,64(%rax)
	mov %r9,72(%rax)
	mov %r10,80(%rax)
	mov %r11,88(%rax)
	mov %r12,96(%rax)
	mov %r13,104(%rax)
	mov %r14,112(%rax)
	mov %r15,120(%rax)

	push %rbx

	mov retaddr+8(%rsp),%rbx /* Save return address (%esp has changed) */
	mov %rbx,0(%rax)

	pop %rbx

	xor %rax,%rax /* Return 0 */
	ret
