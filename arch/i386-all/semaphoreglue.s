/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

	/*    The following functions are guaranteed to preserve	*/
	/*    all registers. But I don't want to write them completely  */
	/*    in assembly - C is generally more readable.		*/
	/*    So I use those stubs to preserve the registers.		*/

	.text
	.balign 16
	.globl	_Exec__ObtainSemaphore
	.type	_Exec__ObtainSemaphore,@function
_Exec__ObtainSemaphore:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	_Exec_ObtainSemaphore
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	_Exec__ReleaseSemaphore
	.type	_Exec__ReleaseSemaphore,@function
_Exec__ReleaseSemaphore:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	_Exec_ReleaseSemaphore
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	_Exec__ObtainSemaphoreShared
	.type	_Exec__ObtainSemaphoreShared,@function
_Exec__ObtainSemaphoreShared:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	_Exec_ObtainSemaphoreShared
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

