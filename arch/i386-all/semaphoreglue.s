/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

#include "machine.i"

/*
    The following functions are guaranteed to preserve
    all registers. But I don't want to write them completely
    in assembly - C is generally more readable.
    So I use those stubs to preserve the registers.
*/

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(_ObtainSemaphore,Exec)
	.type	AROS_SLIB_ENTRY(_ObtainSemaphore,Exec),@function
AROS_SLIB_ENTRY(_ObtainSemaphore,Exec):
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	AROS_SLIB_ENTRY(ObtainSemaphore,Exec)
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	AROS_SLIB_ENTRY(_ReleaseSemaphore,Exec)
	.type	AROS_SLIB_ENTRY(_ReleaseSemaphore,Exec),@function
AROS_SLIB_ENTRY(_ReleaseSemaphore,Exec):
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	AROS_SLIB_ENTRY(ReleaseSemaphore,Exec)
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	AROS_SLIB_ENTRY(_ObtainSemaphoreShared,Exec)
	.type	AROS_SLIB_ENTRY(_ObtainSemaphoreShared,Exec),@function
AROS_SLIB_ENTRY(_ObtainSemaphoreShared,Exec):
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec)
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

