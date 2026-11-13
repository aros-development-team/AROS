/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function siglongjmp()
    Lang: english
*/

	#include "aros/x86_64/asm.h"

	.text
	_ALIGNMENT
	.globl	AROS_CDEFNAME(siglongjmp)
	_FUNCTION(AROS_CDEFNAME(siglongjmp))

	.set	FirstArg, 8 /* Skip Return-Adress */
	.set	env, FirstArg
	.set	val, env+8

AROS_CDEFNAME(siglongjmp):

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
