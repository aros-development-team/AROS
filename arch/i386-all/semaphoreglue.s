#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/08/23 16:49:21  digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#    	I replaced it with .balign which does what we want.
#
#    Revision 1.2  1996/08/01 17:41:19  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	/*    The following functions are guaranteed to preserve	*/
	/*    all registers. But I don't want to write them completely  */
	/*    in assembly - C is generally more readable.		*/
	/*    So I use those stubs to preserve the registers.		*/

	.text
	.balign	16
	.globl	Exec__ObtainSemaphore
	.type	Exec__Obtainsemaphore,@function
Exec__ObtainSemaphore:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	Exec_ObtainSemaphore
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	Exec__ReleaseSemaphore
	.type	Exec__ReleaseSemaphore,@function
Exec__ReleaseSemaphore:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	Exec_ReleaseSemaphore
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

	.globl	Exec__ObtainSemaphoreShared
	.type	Exec__ObtainSemaphoreShared,@function
Exec__ObtainSemaphoreShared:
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	movl	20(%esp),%eax
	pushl	%eax
	movl	20(%esp),%eax
	pushl	%eax
	call	Exec_ObtainSemaphoreShared
	addl	$8,%esp
	popl	%edx
	popl	%ecx
	popl	%eax
	ret

