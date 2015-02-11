/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function sigsetjmp()
    Lang: english
*/

	#include "aros/x86_64/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(sigsetjmp)
	_FUNCTION(AROS_CDEFNAME(sigsetjmp))

AROS_CDEFNAME(sigsetjmp):
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
